#include "application.h"
#include "gpk_bitmap_file.h"
#include "gpk_tcpip.h"
#include "gpk_connection.h"

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

	//SESSION_STAGE_CLIENT_CLOSED					= 0	// sessionClose							()
	//SESSION_STAGE_CLIENT_IDENTIFY						// authorityClientIdentifyRequest		() // Client	-> Authority	// Processed by authority
	//SESSION_STAGE_AUTHORITY_IDENTIFY					// authorityServerIdentifyResponse		() // Authority	-> Client		// Processed by client
	//SESSION_STAGE_CLIENT_REQUEST_SERVICE_START		// sessionClientStart					() // Client	-> Service		// Processed by service
	//SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST		// authorityServiceConfirmClientRequest	() // Service	-> Authority	// Processed by authority
	//SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT			// authorityServerConfirmClientResponse	() // Authority	-> Service		// Processed by service
	//SESSION_STAGE_SERVER_ACCEPT_CLIENT				// sessionServerAccept					() // Service	-> Client		// Processed by client
	//SESSION_STAGE_CLIENT_IDLE							// sessionClientAccepted				() // Client	-> IDLE			// Processed by client/service

static		::gpk::error_t											sessionClientConnect		(::dwh::SUDPSessionClient & client)										{
	uint64_t																totalTime					= 0;
	::gpk::STimer															timer;
	client.UDPClient.AddressConnect										= client.AddressAuthority;
	ree_if(errored(::gpk::clientConnect(client.UDPClient)), "Failed to connect to authority server at: %u.%u.%u.%u:%u.", GPK_IPV4_EXPAND(client.UDPClient.AddressConnect));

	::gpk::array_pod<byte_t>												dataToSend;
	::dwh::authorityClientIdentifyRequest	(client.Client, dataToSend);
	::gpk::connectionPushData(client.UDPClient, client.UDPClient.Queue, dataToSend);
	::gpk::array_pod<byte_t>												dataReceived;
	while(true) {
		::gpk::clientUpdate(client.UDPClient);
		::gpk::mutex_guard														lock_rcv					(client.UDPClient.Queue.MutexReceive);
		for(uint32_t iRcv = 0, countRcv = client.UDPClient.Queue.Received.size(); iRcv < countRcv; ++iRcv) {
			::gpk::ptr_obj<::gpk::SUDPConnectionMessage>							message						= client.UDPClient.Queue.Received[iRcv];
			if(0 == message || 0 == message->Payload.size())
				continue;
			::dwh::SSessionCommand													& command					= *(::dwh::SSessionCommand*)message->Payload.begin();
			switch(command.Command) {
			case ::dwh::SESSION_STAGE_AUTHORITY_IDENTIFY	:	
				info_printf("%s", "Received identify response.");
				dataReceived														= message->Payload;
				break;
			default:
				error_printf("Invalid session command received by client: %u.", (uint32_t)command.Command);
			}
		}
		if(0 != dataReceived.size())
			break;
	}
	::gpk::clientDisconnect(client.UDPClient);
	timer.Frame();
	totalTime																+= timer.LastTimeMicroseconds;
	double																		inSeconds					= totalTime / 1000.0;
	info_printf("---- Public key retrieved in %g milliseconds.", inSeconds);

	client.UDPClient.AddressConnect										= client.AddressServer;
	ree_if(errored(::gpk::clientConnect(client.UDPClient)), "Failed to connect to service's server at: %u.%u.%u.%u:%u.", GPK_IPV4_EXPAND(client.UDPClient.AddressConnect));
	dataToSend.clear();
	gpk_necall(::dwh::sessionClientStart(client.Client, dataReceived, dataToSend), "%s", "Failed to start session.");
	::gpk::connectionPushData(client.UDPClient, client.UDPClient.Queue, dataToSend);
	dataReceived.clear();
	while(true) {
		::gpk::clientUpdate(client.UDPClient);
		for(uint32_t iRcv = 0, countRcv = client.UDPClient.Queue.Received.size(); iRcv < countRcv; ++iRcv) {
			::gpk::ptr_obj<::gpk::SUDPConnectionMessage>							message						= client.UDPClient.Queue.Received[iRcv];
			if(0 == message || 0 == message->Payload.size())
				continue;
			::dwh::SSessionCommand													& command					= *(::dwh::SSessionCommand*)message->Payload.begin();
			switch(command.Command) {
			case ::dwh::SESSION_STAGE_SERVER_ACCEPT_CLIENT	:	
				info_printf("%s", "Received accept response.");
				dataReceived														= message->Payload;
				break;
			default:
				error_printf("Invalid session command received by client: %u.", (uint32_t)command.Command);
			}
		}
		if(0 != dataReceived.size())
			break;
	}
	dataToSend.clear();
	::dwh::sessionClientAccepted(client.Client, dataReceived, dataToSend);
	timer.Frame();
	totalTime																+= timer.LastTimeMicroseconds;
	inSeconds																= totalTime / 1000.0;
	info_printf("---- Session keys set in %g milliseconds.", inSeconds);
	info_printf("RSA Private  : %llu.", client.Client.RSAKeys.Private  );
	info_printf("RSA PrivateN : %llu.", client.Client.RSAKeys.PrivateN );
	info_printf("RSA Public   : %llu.", client.Client.RSAKeys.Public   );
	info_printf("RSA PublicN  : %llu.", client.Client.RSAKeys.PublicN  );
	info_printf("Ardell       : %llx.", client.Client.KeySymmetric[0]  );
	for(uint32_t i=1; i < 5; ++i)
		info_printf("AES[%u]        : %llx.", i - 1, client.Client.KeySymmetric[i]);
	return 0;
}

static		void													sessionClientConnect		(void * pclient)														{
	::dwh::SUDPSessionClient												& client					= *(::dwh::SUDPSessionClient *)pclient;
	error_if(errored(::sessionClientConnect(client)), "Failed to connect to server. %s", "");
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

	app.Client.UDPClient.AddressConnect									= {};
	::gpk::tcpipAddress(32765, 0, ::gpk::TRANSPORT_PROTOCOL_UDP, app.Client.AddressServer	);
	::gpk::tcpipAddress(32766, 0, ::gpk::TRANSPORT_PROTOCOL_UDP, app.Client.AddressAuthority);
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
	if(app.Client.UDPClient.State == ::gpk::UDP_CONNECTION_STATE_IDLE) {
		error_if(errored(::gpk::clientUpdate(app.Client.UDPClient)), "%s", "Unknown error.");
		if(app.Client.Client.Stage == ::dwh::SESSION_STAGE_CLIENT_IDLE) {
			// Do work
		}
	}
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