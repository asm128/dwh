#include "application.h"
#include "gpk_bitmap_file.h"
#include "gpk_tcpip.h"
#include "gpk_connection.h"
#include "gpk_grid_scale.h"

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
		gui.Zoom.ZoomLevel													+= app.Framework.Input->MouseCurrent.Deltas.z * (1.0 / (120 * 4));
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

	static uint32_t redundancy = 0;
	++redundancy;
	bool forceSendInput = (0 == (redundancy % 3));

	if(app.Client.UDPClient.State == ::gpk::UDP_CONNECTION_STATE_IDLE) {
		if(app.Client.Client.Stage == ::dwh::SESSION_STAGE_CLIENT_IDLE) {
			::gpk::array_obj<::gpk::ptr_obj<::gpk::SUDPConnectionMessage>>	queue;
			{
				// Do work
				::gpk::mutex_guard												lockQeueue					(app.Client.UDPClient.Queue.MutexReceive);
				queue														= app.Client.UDPClient.Queue.Received;
				app.Client.UDPClient.Queue.Received.clear();
			}
			for(uint32_t iRecv = 0; iRecv < queue.size(); ++iRecv) {
				if(7 != queue[iRecv]->Payload[0])
					continue;
				uint32_t														line						= *(uint16_t*)&queue[iRecv]->Payload[1];
				//info_printf("Received line: %u.", line);
				::gpk::SCoord2<uint16_t>										remoteScreenSize			= *(::gpk::SCoord2<uint16_t>*)&queue[iRecv]->Payload[3];
				{
					::gpk::mutex_guard														lock					(app.LockRender);
					app.OffscreenRemote->resize(remoteScreenSize.Cast<uint32_t>());
				}
				memcpy(app.OffscreenRemote->Color.View[line].begin(), &queue[iRecv]->Payload[7], remoteScreenSize.x * sizeof(::gpk::SColorBGRA));
			}
			::gpk::array_pod<byte_t>											packetInputs;
			packetInputs.push_back(7); 
			bool changed = false;
			if(false == forceSendInput && 0 == memcmp(&app.Framework.Input->KeyboardCurrent, &app.Framework.Input->KeyboardPrevious, sizeof(::gpk::SInputKeyboard))) 
				packetInputs.push_back(0);
			else {
				packetInputs.push_back(1);
				packetInputs.append((const byte_t*)&app.Framework.Input->KeyboardCurrent, sizeof(::gpk::SInputKeyboard));
				changed = true;
			}

			if(false == forceSendInput && 0 == memcmp(&app.Framework.Input->MouseCurrent, &app.Framework.Input->MousePrevious, sizeof(::gpk::SInputMouse))) 
				packetInputs.push_back(0);
			else {
				packetInputs.push_back(1);
				packetInputs.append((const byte_t*)&app.Framework.Input->MouseCurrent	, sizeof(::gpk::SInputMouse));
				changed = true;
			}
			if(changed)
				::gpk::connectionPushData(app.Client.UDPClient, app.Client.UDPClient.Queue, packetInputs);
		}
	}
	char buffer[256] = {};
	sprintf_s(buffer, "Last frame seconds: %g.", app.Framework.FrameInfo.Seconds.LastFrame);
	SetWindowText(app.Framework.MainDisplay.PlatformDetail.WindowHandle, buffer);

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