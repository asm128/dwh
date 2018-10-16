#include "application.h"
#include "gpk_bitmap_file.h"
#include "gpk_tcpip.h"

//#define GPK_AVOID_LOCAL_APPLICATION_MODULE_MODEL_EXECUTABLE_RUNTIME
#include "gpk_app_impl.h"

GPK_DEFINE_APPLICATION_ENTRY_POINT(::gme::SApplication, "Module Explorer");

			::gpk::error_t											cleanup						(::gme::SApplication & app)						{ 
	::gpk::serverStop(app.Server);
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
	::gpk::serverStart(app.Server, 9998);

	gpk_necall(::dwh::loadClientModule(app.ClientModule, "dwh_client.dll"), "Failed to load client dll.");

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
	{
		::gpk::mutex_guard														lock						(app.Server.Mutex);
		for(uint32_t iClient = 0; iClient < app.Server.Clients.size(); ++iClient) {
			::gpk::mutex_guard														lockRecv					(app.Server.Clients[iClient]->Queue.MutexReceive);
			for(uint32_t iMessage = 0; iMessage < app.Server.Clients[iClient]->Queue.Received.size(); ++iMessage) {
				info_printf("Received: %s.", app.Server.Clients[iClient]->Queue.Received[iMessage]->Payload.begin());
			}
			app.Server.Clients[iClient]->Queue.Received.clear();
		}
	}

	::gpk::SIPv4								localAddress				= {};
	::gpk::tcpipAddress(0, 0, ::gpk::TRANSPORT_PROTOCOL_UDP, localAddress);

	const int32_t								idClient					= app.ClientModule.Create();
	rww_if(errored(idClient), "%s", "Error");
	char										token		[1024]			= {};
	uint32_t									tokenSize					= 0;
	if errored(app.ClientModule.Connect(idClient, GPK_IPV4_EXPAND(localAddress), &tokenSize, token)) {
		app.ClientModule.Destroy(idClient);
		error_printf("%s.", "Error");
		return -1;
	}
	uint64_t									idOperation					= (uint64_t)-1LL;
	uint32_t									storeResult					= 0;
	{
		static constexpr	const char				testStore	[4096]			= {};
		storeResult								= app.ClientModule.Store(idClient, ::gpk::size(testStore), testStore, &idOperation);
	}
	if (errored(storeResult) || errored(idOperation)) {
		error_printf("Store failed. Store result: %u. Operation id: %llu.", storeResult, idOperation);
		error_if(errored(app.ClientModule.Disconnect(idClient)), "%s.", "Error");
	}
	else {
		byte_t										testLoad	[4096]			= {};
		uint32_t									sizeLoad					= ::gpk::size(testLoad);
		error_if(errored(app.ClientModule.Load(idClient, &sizeLoad, testLoad, idOperation)), "Failed to load data for operation id: %llu.", idOperation);
		error_if(errored(app.ClientModule.Disconnect(idClient)), "%s.", "Error");
	}
	error_if(errored(app.ClientModule.Destroy(idClient)), "Failed to destroy client widh id: %i", idClient);
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