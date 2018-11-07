#include "application.h"
#include "gpk_bitmap_file.h"
#include "gpk_tcpip.h"
#include "gpk_connection.h"
#include "gpk_grid_scale.h"

//#define GPK_AVOID_LOCAL_APPLICATION_MODULE_MODEL_EXECUTABLE_RUNTIME
#include "gpk_app_impl.h"

GPK_DEFINE_APPLICATION_ENTRY_POINT(::gme::SApplication, "Module Explorer");

			::gpk::error_t											cleanup						(::gme::SApplication & app)						{ 
	::gpk::clientDisconnect(app.Server.UDPClient);
	::gpk::serverStop(app.Server.UDPServer);
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
	{ // Create exit button.
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
	}

	::gpk::tcpipInitialize();
	::gpk::serverStart(app.Server.UDPServer, 32765);
	return 0; 
}


			::gpk::error_t											getBuffer									(::SOffscreenPlatformDetail& offscreenCache, ::HDC hdc, int width, int height, ::gpk::view_grid<::gpk::SColorBGRA>& colorArray)				{
	const uint32_t															bytesToCopy									= sizeof(::RGBQUAD) * colorArray.size();
	::SOffscreenPlatformDetail												& offscreenDetail							= offscreenCache;//{};
	const ::gpk::SCoord2<uint32_t>											metricsSource								= colorArray.metrics();
	if( 0 == offscreenDetail.BitmapInfo || 
		( metricsSource.x != (uint32_t)offscreenDetail.BitmapInfo->bmiHeader.biWidth  
		|| metricsSource.y != (uint32_t)offscreenDetail.BitmapInfo->bmiHeader.biHeight 
		)
	) {
		offscreenDetail.BitmapInfoSize										= sizeof(::BITMAPINFO) + bytesToCopy;
		if(offscreenDetail.BitmapInfo)
			free(offscreenDetail.BitmapInfo);
		ree_if(0 == (offscreenDetail.BitmapInfo = (::BITMAPINFO*)::malloc(offscreenDetail.BitmapInfoSize)), "malloc(%u) failed! Out of memory?", offscreenDetail.BitmapInfoSize);

		offscreenDetail.BitmapInfo->bmiHeader.biSize							= sizeof(::BITMAPINFO);
		offscreenDetail.BitmapInfo->bmiHeader.biWidth							= metricsSource.x;
		offscreenDetail.BitmapInfo->bmiHeader.biHeight							= metricsSource.y;
		offscreenDetail.BitmapInfo->bmiHeader.biPlanes							= 1;
		offscreenDetail.BitmapInfo->bmiHeader.biBitCount						= 32;
		offscreenDetail.BitmapInfo->bmiHeader.biCompression						= BI_RGB;
		offscreenDetail.BitmapInfo->bmiHeader.biSizeImage						= bytesToCopy;
		offscreenDetail.BitmapInfo->bmiHeader.biXPelsPerMeter					= 0x0ec4; // Paint and PSP use these values.
		offscreenDetail.BitmapInfo->bmiHeader.biYPelsPerMeter					= 0x0ec4; // Paint and PSP use these values.
		offscreenDetail.BitmapInfo->bmiHeader.biClrUsed							= 0; 
		offscreenDetail.BitmapInfo->bmiHeader.biClrImportant					= 0;

		offscreenDetail.IntermediateDeviceContext								= ::CreateCompatibleDC(hdc);    // <- note, we're creating, so it needs to be destroyed
		char																		* ppvBits									= 0;
		reterr_error_if(0 == (offscreenDetail.IntermediateBitmap = ::CreateDIBSection(offscreenDetail.IntermediateDeviceContext, offscreenDetail.BitmapInfo, DIB_RGB_COLORS, (void**) &ppvBits, NULL, 0)), "%s", "Failed to create intermediate dib section.");
	}
	::HBITMAP																	hBmpOld										= (::HBITMAP)::SelectObject(offscreenDetail.IntermediateDeviceContext, offscreenDetail.IntermediateBitmap);    // <- altering state
	//error_if(FALSE == ::BitBlt(hdc, 0, 0, width, height, offscreenDetail.IntermediateDeviceContext, 0, 0, SRCCOPY), "%s", "Not sure why would this happen but probably due to mismanagement of the target size or the system resources. I've had it failing when I acquired the device too much and never released it.");
	::SetStretchBltMode(hdc, COLORONCOLOR);
	error_if(FALSE == ::StretchBlt(offscreenDetail.IntermediateDeviceContext, 0, 0, width, height, hdc, 0, 0, metricsSource.x, metricsSource.y, SRCCOPY), "%s", "Not sure why would this happen but probably due to mismanagement of the target size or the system resources. I've had it failing when I acquired the device too much and never released it.");
	reterr_error_if(0 == ::GetDIBits(offscreenDetail.IntermediateDeviceContext, offscreenDetail.IntermediateBitmap, 0, height, offscreenDetail.BitmapInfo->bmiColors, offscreenDetail.BitmapInfo, DIB_RGB_COLORS), "%s", "Cannot copy bits into dib section.");

	for(uint32_t y = 0, yMax = metricsSource.y; y < yMax; ++y)
		memcpy(colorArray[y].begin(), &offscreenDetail.BitmapInfo->bmiColors[(metricsSource.y - 1 - y) * metricsSource.x], metricsSource.x * sizeof(::gpk::SColorBGRA));

	::SelectObject(hdc, hBmpOld);	// put the old bitmap back in the DC (restore state)
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


	::gpk::array_pod<uint16_t>	linesToSend;
	{
		::gpk::mutex_guard														lock					(app.LockRender);
		RECT																	rect					= {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
		//GetWindowRect(0, &rect);
		if(app.DesktopImage.metrics().x != (uint32_t)rect.right || app.DesktopImage.metrics().y != (uint32_t)rect.bottom) {
			app.DesktopImage		.resize({(uint32_t)rect.right, (uint32_t)rect.bottom});
			app.DesktopImagePrevious.resize({(uint32_t)rect.right, (uint32_t)rect.bottom});
		}
		::gpk::array_pod<::gpk::SColorBGRA>										& pixels				= app.DesktopImage.Texels;
		::gpk::view_grid<::gpk::SColorBGRA>										view					= {pixels.begin(), (uint32_t)rect.right, (uint32_t)rect.bottom};

		HDC																		dc						= GetDC(0);
		::getBuffer(app.OffscreenDetail, dc, rect.right, rect.bottom, view);
		ReleaseDC(0, dc);
	}
	for(uint16_t iLine = 0; iLine < (uint16_t)app.DesktopImage.metrics().y; ++iLine) {
		if(memcmp(app.DesktopImage.View[iLine].begin(), app.DesktopImagePrevious.View[iLine].begin(), app.DesktopImage.View.metrics().x * sizeof(::gpk::SColorBGRA))) 
			linesToSend.push_back(iLine);
	}

	//info_printf("%s", "Lines to send: ");
	::gpk::array_pod<byte_t>												lineCommand;
	for(uint32_t iLine = 0; iLine < linesToSend.size(); ++iLine) {
		//info_printf("Line (%u): %u", iLine, (uint32_t)linesToSend[iLine]);
		lineCommand.resize(app.DesktopImage.metrics().x * sizeof(::gpk::SColorBGRA) + sizeof(::gpk::SCoord2<uint16_t>) + sizeof(uint16_t) + 1);
		lineCommand[0]														= 7;
		*(uint16_t*)&lineCommand[1]											= linesToSend[iLine];
		{
			::gpk::mutex_guard														lock					(app.LockRender);
			*(::gpk::SCoord2<uint16_t>*)&lineCommand[3]							= app.DesktopImage.metrics().Cast<uint16_t>();
			memcpy(&lineCommand[7], app.DesktopImage.View[linesToSend[iLine]].begin(), app.DesktopImage.metrics().x * sizeof(::gpk::SColorBGRA));
		}

		{
			::gpk::mutex_guard														lock					(app.Server.UDPServer.Mutex);
			for(uint32_t iClient = 0, countClients = app.Server.UDPServer.Clients.size(); iClient < countClients; ++iClient) {
				if(app.Server.Server.Clients[iClient].Stage != ::dwh::SESSION_STAGE_SERVER_ACCEPT_CLIENT)
					continue;
				if(0 == app.Server.UDPServer.Clients[iClient])
					continue;
				::gpk::SUDPConnection													& connection				= *app.Server.UDPServer.Clients[iClient];
				::gpk::connectionPushData(connection, connection.Queue, lineCommand);
				::gpk::sleep(0);
			}
		}
	}

	{
		::gpk::mutex_guard														lock					(app.LockRender);
		memcpy(app.DesktopImagePrevious.Texels.begin(), app.DesktopImage.Texels.begin(), app.DesktopImage.View.metrics().x * app.DesktopImage.View.metrics().y * sizeof(::gpk::SColorBGRA));
	}


	::dwh::sessionServerUpdate(app.Server);

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
		::gpk::mutex_guard														lock					(app.LockGUI);
		::gpk::controlDrawHierarchy(app.Framework.GUI, 0, target->Color.View);
	}
	{
		::gpk::mutex_guard														lock					(app.LockRender);
		::gpk::grid_scale(target->Color.View, app.DesktopImage.View);
		app.Offscreen														= target;
	}

	//timer.Frame();
	//warning_printf("Draw time: %f.", (float)timer.LastTimeSeconds);
	return 0; 
}
