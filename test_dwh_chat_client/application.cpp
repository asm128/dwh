#include "application.h"
#include "gpk_bitmap_file.h"
#include "gpk_tcpip.h"
#include "gpk_connection.h"
#include "gpk_grid_scale.h"
#include "dwh_stream.h"


//#define GPK_AVOID_LOCAL_APPLICATION_MODULE_MODEL_EXECUTABLE_RUNTIME
#include "gpk_app_impl.h"

#if defined (GPK_WINDOWS)
#	include <process.h>	// for _beginthread
#endif

GPK_DEFINE_APPLICATION_ENTRY_POINT(::gme::SApplication, "Module Explorer");

			::gpk::error_t											cleanup						(::gme::SApplication & app)												{ 
	::gpk::clientDisconnect(app.Client.UDPClient);
	::gpk::mainWindowDestroy(app.Framework.MainDisplay);
	::gpk::tcpipShutdown();
	return 0; 
}

static		void													sessionClientConnect		(void * pclient)														{
	::dwh::SUDPSessionClient												& client					= *(::dwh::SUDPSessionClient *)pclient;
	::gpk::tcpipAddress(32765, 0, ::gpk::TRANSPORT_PROTOCOL_UDP, client.AddressServer	);
	::gpk::tcpipAddress(32766, 0, ::gpk::TRANSPORT_PROTOCOL_UDP, client.AddressAuthority);
	//client.AddressServer	= {{192,168,1,4},32765};
	//client.AddressAuthority	= {{192,168,1,4},32766};
	//client.AddressServer		= {{192, 168, 1, 54}, 32765};
	//client.AddressAuthority		= {{192, 168, 1, 54}, 32766};
	client.Client.IdServer												= 0;
	error_if(errored(::dwh::sessionClientConnect(client)), "Failed to connect to server. %s", "");
}

			::gpk::error_t											setup						(::gme::SApplication & app)												{ 
	::gpk::SFramework														& framework					= app.Framework;
	::gpk::SDisplay															& mainWindow				= framework.MainDisplay;
	framework.Input.create();
	error_if(errored(::gpk::mainWindowCreate(mainWindow, framework.RuntimeValues.PlatformDetail, framework.Input)), "Failed to create main window!!! Why?????!?!?!?!?");
	::gpk::SGUI																& gui						= framework.GUI;
	app.IdExit															= ::gpk::controlCreate(gui);
	::gpk::SControl															& controlExit				= gui.Controls.Controls[app.IdExit];
	controlExit.Area													= {{0, 0}, {64, 20}};
	controlExit.Border													= {1, 1, 1, 1};
	controlExit.Margin													= {1, 1, 1, 1};
	controlExit.Align													= ::gpk::ALIGN_BOTTOM_RIGHT;
	::gpk::SControlText														& controlText				= gui.Controls.Text[app.IdExit];
	controlText.Text													= "Exit";
	controlText.Align													= ::gpk::ALIGN_CENTER;
	::gpk::SControlConstraints												& controlConstraints		= gui.Controls.Constraints[app.IdExit];
	controlConstraints.AttachSizeToText.y								= app.IdExit;
	controlConstraints.AttachSizeToText.x								= app.IdExit;
	::gpk::controlSetParent(gui, app.IdExit, -1);
	::gpk::tcpipInitialize();

	::gpk::serverStart(app.Server, 31321);
	_beginthread(::sessionClientConnect, 0, &app.Client);
	return 0; 
}


			::gpk::error_t											update						(::gme::SApplication & app, bool exitSignal)	{ 
	::gpk::STimer															timer;
	retval_info_if(::gpk::APPLICATION_STATE_EXIT, exitSignal, "%s", "Exit requested by runtime.");
	{
		::gpk::mutex_guard														lock						(app.LockRender);
		app.Framework.MainDisplayOffscreen									= app.Offscreen;
	}
	::gpk::SFramework														& framework					= app.Framework;
	retval_info_if(::gpk::APPLICATION_STATE_EXIT, ::gpk::APPLICATION_STATE_EXIT == ::gpk::updateFramework(app.Framework), "Exit requested by framework update.");

	::gpk::SGUI																& gui						= framework.GUI;
	{
		::gpk::mutex_guard														lock						(app.LockGUI);
		::gpk::guiProcessInput(gui, *app.Framework.Input);
	}
	if(app.Framework.Input->MouseCurrent.Deltas.z) {
		gui.Zoom.ZoomLevel													+= app.Framework.Input->MouseCurrent.Deltas.z * (1.0 / (120LL * 4));
		::gpk::guiUpdateMetrics(gui, app.Offscreen->Color.metrics(), true);
	}
 
	for(uint32_t iControl = 0, countControls = gui.Controls.Controls.size(); iControl < countControls; ++iControl) {
		const ::gpk::SControlState												& controlState				= gui.Controls.States[iControl];
		if(controlState.Unused || controlState.Disabled)
			continue;
		if(controlState.Execute) {
			info_printf("Executed %u.", iControl);
			if(iControl == (uint32_t)app.IdExit)
				return 1;
		}
	}

	//error_if(errored(::dwh::authorityUpdate(app.Server.UDPServer, app.Server.Authority)), "%s.", "Unknown error");
	if(app.Client.UDPClient.State == ::gpk::UDP_CONNECTION_STATE_IDLE)
		error_if(errored(::gpk::clientUpdate(app.Client.UDPClient)), "%s", "Unknown error.");

	static uint32_t													redundancy						= 0;
	++redundancy;
	bool															forceSendInput					= (0 == (redundancy % 3));
	forceSendInput = 0;

	//::gpk::array_pod<uint16_t>										lineCodecCache					= {};
	if(app.Client.UDPClient.State == ::gpk::UDP_CONNECTION_STATE_IDLE) {
		if(app.Client.Client.Stage == ::dwh::SESSION_STAGE_CLIENT_IDLE) {
			::gpk::array_obj<::gpk::ptr_obj<::gpk::SUDPConnectionMessage>>	queue;
			{ // Do work
				::gpk::mutex_guard												lockQeueue					(app.Client.UDPClient.Queue.MutexReceive);
				queue														= app.Client.UDPClient.Queue.Received;
				app.Client.UDPClient.Queue.Received.clear();
			}
			// --------------- Receive lines
			for(uint32_t iRecv = 0; iRecv < queue.size(); ++iRecv) {
				::gpk::ptr_nco<::gpk::SUDPConnectionMessage>					linemsg						= queue[iRecv];
				if(::dwh::SESSION_STAGE_CLIENT_IDLE != linemsg->Payload[0])
					continue;
				::dwh::SLineHeader												& lineHeader				= *(::dwh::SLineHeader*)linemsg->Payload.begin();
				uint32_t														bits16						= lineHeader.Format.Bits16;	
				uint32_t														line						= lineHeader.Area.Offset.y;	
				::gpk::SCoord2<uint16_t>										remoteScreenSize			= lineHeader.ImageSize;		
				{
					::gpk::mutex_guard												lock						(app.LockRender);
					app.OffscreenRemote->resize(remoteScreenSize.Cast<uint32_t>());
				}
				static constexpr const uint32_t lineHeaderSize = sizeof(::dwh::SLineHeader);
				if(0 == bits16) { //info_printf("Received line: %u.", line);
					memcpy(app.OffscreenRemote->Color.View[line].begin(), &linemsg->Payload[lineHeaderSize], remoteScreenSize.x * sizeof(::gpk::SColorBGRA));
				}
				else {
					if(1 == bits16) {
						const uint32_t													payloadLineSize				= (linemsg->Payload.size() - lineHeaderSize) / 2;
						::gpk::view_array<const uint16_t>								pixels16					= {(const uint16_t*)&linemsg->Payload[lineHeaderSize], payloadLineSize};
						for(uint32_t iPix = 0; iPix < remoteScreenSize.x; ++iPix) {
							app.OffscreenRemote->Color.View[line][iPix]					= {(uint8_t)((pixels16[iPix] & 0x1F) / 31.0 * 255), (uint8_t)(((pixels16[iPix] >> 5) & 0x3F) / 63.0 * 255), (uint8_t)(((pixels16[iPix] >> 11) & 0x1F) / 31.0 * 255), 255};
						}
					}
				}
			}

			// --------------- Send user input
			::gpk::array_pod<byte_t>										packetInputs;
			packetInputs.push_back(::dwh::SESSION_STAGE_CLIENT_IDLE); 
			bool															changed							= false;
			::dwh::SInputHeader												header;
			header.Keyboard												= forceSendInput || 0 == memcmp(&app.Framework.Input->KeyboardCurrent	, &app.Framework.Input->KeyboardPrevious, sizeof(::gpk::SInputKeyboard));
			header.Mouse												= forceSendInput || 0 == memcmp(&app.Framework.Input->MouseCurrent		, &app.Framework.Input->MousePrevious	, sizeof(::gpk::SInputMouse));
			header.OffscreenSize										= app.Offscreen->Color.metrics().Cast<uint16_t>();
			packetInputs.append((const byte_t*)&header, sizeof(::dwh::SInputHeader));
			if(header.Keyboard) {
				packetInputs.append((const byte_t*)&app.Framework.Input->KeyboardCurrent, sizeof(::gpk::SInputKeyboard));
				changed														= true;
			}
			if(header.Mouse) {
				packetInputs.append((const byte_t*)&app.Framework.Input->MouseCurrent	, sizeof(::gpk::SInputMouse));
				changed														= true;
			}
			if(changed)
				::gpk::connectionPushData(app.Client.UDPClient, app.Client.UDPClient.Queue, packetInputs, true, true, 10);
		}
	}
	char buffer[256] = {};
	sprintf_s(buffer, "Last frame seconds: %g.", app.Framework.FrameInfo.Seconds.LastFrame);
	SetWindowText(app.Framework.MainDisplay.PlatformDetail.WindowHandle, buffer);

	::gpk::sleep(1);
	//timer.Frame();
	//warning_printf("Update time: %f.", (float)timer.LastTimeSeconds);
	return 0; 
}

			::gpk::error_t											draw					(::gme::SApplication & app)						{ 
	::gpk::STimer															timer;
	app;
	::gpk::ptr_obj<::gpk::SRenderTarget<::gpk::SColorBGRA, uint32_t>>		target;
	target.create();
	target->resize(app.Framework.MainDisplay.Size, {0xFF, 0x40, 0x7F, 0xFF}, (uint32_t)-1);
	{
		::gpk::mutex_guard														lock					(app.LockRender);
		if(app.OffscreenRemote)
			::gpk::grid_scale(target->Color.View, app.OffscreenRemote->Color.View);
	}
	{
		::gpk::mutex_guard														lock					(app.LockGUI);
		::gpk::controlDrawHierarchy(app.Framework.GUI, 0, target->Color.View);
	}
	{
		::gpk::mutex_guard														lock					(app.LockRender);
		app.Offscreen														= target;
	}
	//timer.Frame();
	//warning_printf("Draw time: %f.", (float)timer.LastTimeSeconds);
	return 0; 
}