#include "application.h"
#include "gpk_bitmap_file.h"
#include "gpk_tcpip.h"
#include "gpk_connection.h"

//#define GPK_AVOID_LOCAL_APPLICATION_MODULE_MODEL_EXECUTABLE_RUNTIME
#include "gpk_app_impl.h"

GPK_DEFINE_APPLICATION_ENTRY_POINT(::gme::SApplication, "Module Explorer");

			::gpk::error_t											cleanup						(::gme::SApplication & app)						{ 
	::gpk::serverStop(app.UDPServer);
	::gpk::mainWindowDestroy(app.Framework.MainDisplay);
	::gpk::tcpipShutdown();
	return 0; 
}

			::gpk::error_t											setup						(::gme::SApplication & app)						{ 
	::gpk::SFramework														& framework					= app.Framework;
	::gpk::SDisplay															& mainWindow				= framework.MainDisplay;
	framework.Input.create();
	error_if(errored(::gpk::mainWindowCreate(mainWindow, framework.RuntimeValues.PlatformDetail, framework.Input)), "Failed to create main window why?????!?!?!?!?");
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
	::gpk::serverStart(app.UDPServer, 32765);
	return 0; 
}

			::gpk::error_t											update						(::gme::SApplication & app, bool exitSignal)	{ 
	::gpk::STimer															timer;
	retval_info_if(::gpk::APPLICATION_STATE_EXIT, exitSignal, "Exit requested by runtime.");
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

	::gpk::array_pod<byte_t>												response;
	{
		::gpk::mutex_guard														lock						(app.UDPServer.Mutex);
		//SESSION_STAGE_CLIENT_CLOSED					= 0	// sessionClose							()
		//SESSION_STAGE_CLIENT_IDENTIFY						// authorityClientIdentifyRequest		() // Client	-> Authority	// Processed by authority
		//SESSION_STAGE_AUTHORITY_IDENTIFY					// authorityServerIdentifyResponse		() // Authority	-> Client		// Processed by client
		//SESSION_STAGE_CLIENT_REQUEST_SERVICE_START		// sessionClientStart					() // Client	-> Service		// Processed by service
		//SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST		// authorityServiceConfirmClientRequest	() // Service	-> Authority	// Processed by authority
		//SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT			// authorityServerConfirmClientResponse	() // Authority	-> Service		// Processed by service
		//SESSION_STAGE_SERVER_ACCEPT_CLIENT				// sessionServerAccept					() // Service	-> Client		// Processed by client
		//SESSION_STAGE_CLIENT_IDLE							// sessionClientAccepted				() // Client	-> IDLE			// Processed by client/service
		for(uint32_t iClient = 0; iClient < app.UDPServer.Clients.size(); ++iClient) {
			::gpk::ptr_nco<::gpk::SUDPConnection>									pclient						= app.UDPServer.Clients[iClient];
			if(0 == pclient || pclient->Socket == INVALID_SOCKET || pclient->State == ::gpk::UDP_CONNECTION_STATE_DISCONNECTED)
				continue;
			::gpk::SUDPConnection													& client					= *pclient;
			::gpk::array_obj<::gpk::ptr_obj<::gpk::SUDPConnectionMessage>>			received;
			{
				::gpk::mutex_guard														lockRecv					(client.Queue.MutexReceive);
				received															= client.Queue.Received;
				client.Queue.Received.clear();
			}
			for(uint32_t iMessage = 0; iMessage < received.size(); ++iMessage) {
				::gpk::ptr_nco<::gpk::SUDPConnectionMessage>							pmsg						= received[iMessage];
				if(0 == pmsg || 0 == pmsg->Payload.size())
					continue;
				::gpk::SUDPConnectionMessage											& msg						= *pmsg;
				info_printf("Received: %s.", msg.Payload.begin());
				response.clear();
				::dwh::SSessionCommand													command						= *(::dwh::SSessionCommand*)&msg.Payload[0];
				switch(command.Command) {																			  
				case ::dwh::SESSION_STAGE_CLIENT_REQUEST_SERVICE_START	: 
					ce_if(errored(::dwh::authorityServiceConfirmClientRequest(app.SessionServer, msg.Payload, response)), "Failed to process client command: %u.", (uint32_t)command.Command); 
					ce_if(errored(::gpk::connectionPushData(app.UDPClient, app.UDPClient.Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command.Command); 
					break;
				//case ::dwh::SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT		: 
				//	ce_if(errored(::dwh::sessionServerAccept(app.SessionServer, msg.Payload, response)), "Failed to process server command: %u.", (uint32_t)msg.Payload[0]); 
				//	ce_if(errored(::gpk::connectionPushData(client, client.Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command); break;
				default: 
					error_printf("Unrecognized session command: %u.");
					break;
				}
			}
		}
	}

	::gpk::SUDPConnection													& client					= app.UDPClient;
	::gpk::array_obj<::gpk::ptr_obj<::gpk::SUDPConnectionMessage>>			received;
	{
		::gpk::mutex_guard														lockRecv					(client.Queue.MutexReceive);
		received															= client.Queue.Received;
		client.Queue.Received.clear();
	}
	for(uint32_t iMessage = 0; iMessage < received.size(); ++iMessage) {
		::gpk::ptr_nco<::gpk::SUDPConnectionMessage>							pmsg						= received[iMessage];
		if(0 == pmsg || 0 == pmsg->Payload.size())
			continue;
		::gpk::SUDPConnectionMessage											& msg						= *pmsg;
		info_printf("Received: %s.", msg.Payload.begin());
		response.clear();
		::dwh::SSessionCommand													command						= *(::dwh::SSessionCommand*)&msg.Payload[0];
		int32_t																	indexClientAccepted			= -1;
		switch(command.Command) {
		//case ::dwh::SESSION_STAGE_CLIENT_REQUEST_SERVICE_START	: 
		//	ce_if(errored(::dwh::authorityServiceConfirmClientRequest	(app.SessionServer, msg.Payload, response)), "Failed to process client command: %u.", (uint32_t)msg.Payload[0]); 
		//	ce_if(errored(::gpk::connectionPushData						(app.UDPClient, app.UDPClient.Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command); 
		//	break;
		case ::dwh::SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT		: 
			ce_if(errored(indexClientAccepted = ::dwh::sessionServerAccept(app.SessionServer, msg.Payload, response)), "Failed to process server command: %u.", (uint32_t)command.Command); 
			ce_if(app.SessionServer.Clients.size() <= (uint32_t)indexClientAccepted, "Invalid client index: %i.", indexClientAccepted);

			for(uint32_t iClient = 0; iClient < app.SessionServer.Clients.size(); ++iClient) {
			}
			ce_if(errored(::gpk::connectionPushData(client, client.Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command.Command); 
			break;
		default: 
			error_printf("Unrecognized session command: %u.");
			break;
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