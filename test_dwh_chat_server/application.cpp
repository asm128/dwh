#include "application.h"
#include "gpk_bitmap_file.h"
#include "gpk_tcpip.h"
#include "gpk_connection.h"
#include "gpk_grid_scale.h"
#include "gpk_chrono.h"

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


	app.RemoteInput = {};
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
		//::gpk::guiProcessInput(gui, *app.Framework.Input);
		//::gpk::guiProcessInput(gui, app.RemoteInput);
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
	RECT																	rect					= {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
	//::gpk::SCoord2<uint32_t>												finalImageSize			= {320, 200};//{(uint32_t)rect.right / 2, (uint32_t)rect.bottom / 2};
	::gpk::SCoord2<uint32_t>												finalImageSize			= {(uint32_t)rect.right / 2, (uint32_t)rect.bottom / 2};
	//::gpk::SCoord2<uint32_t>												finalImageSize			= {640, 360};
	{
		::gpk::mutex_guard														lock					(app.LockRender);
		//GetWindowRect(0, &rect);
		if(app.DesktopImage.metrics() != finalImageSize) {
			app.DesktopImage		.resize(finalImageSize);
			app.DesktopImagePrevious.resize(finalImageSize);
		}
	}


	::gpk::SImage<::gpk::SColorBGRA>										temp;
	temp.resize((uint32_t)rect.right, (uint32_t)rect.bottom);
	//::gpk::array_pod<::gpk::SColorBGRA>										& pixels				= app.DesktopImage.Texels;
	::gpk::view_grid<::gpk::SColorBGRA>										& view					= temp.View;//{pixels.begin(), (uint32_t)rect.right, (uint32_t)rect.bottom};
	HDC																		dc						= GetDC(0);
	::getBuffer(app.OffscreenDetail, dc, rect.right, rect.bottom, view);
	ReleaseDC(0, dc);

	::gpk::grid_scale(app.DesktopImage.View, view);

	static int even = 0;
	++even;
	even %= 2;

	for(uint16_t iLine = 0; iLine < (uint16_t)app.DesktopImage.metrics().y; ++iLine) {
		if(memcmp(app.DesktopImage.View[iLine].begin(), app.DesktopImagePrevious.View[iLine].begin(), app.DesktopImage.View.metrics().x * sizeof(::gpk::SColorBGRA))) 
			memcpy(app.DesktopImagePrevious.View[iLine].begin(), app.DesktopImage.View[iLine].begin(), app.DesktopImage.View.metrics().x * sizeof(::gpk::SColorBGRA));
			//if(even && 0 == (iLine % 2))
				linesToSend.push_back(iLine);
			//else if((iLine & 2) && !even)
			//	linesToSend.push_back(iLine);
	}

	//static uint16_t															iRedundancy						= 0;
	//iRedundancy															+= 1;
	//iRedundancy															%= app.DesktopImage.metrics().y;
	//linesToSend.push_back(iRedundancy);

	::gpk::SImage<::gpk::SColorBGRA>										& finalImage	= app.DesktopImage;
	//finalImage.resize(app.DesktopImage.metrics() / 2);
	//::gpk::grid_scale(finalImage.View, app.DesktopImage.View);

	//info_printf("%s", "Lines to send: ");
	::gpk::array_pod<byte_t>												lineCommand;
	for(uint32_t iLine = 0; iLine < linesToSend.size(); ++iLine) {
		//info_printf("Line (%u): %u", iLine, (uint32_t)linesToSend[iLine]);
		lineCommand.resize(finalImage.metrics().x * sizeof(::gpk::SColorBGRA) + sizeof(::gpk::SCoord2<uint16_t>) + sizeof(uint16_t) + 1);
		lineCommand[0]														= 7;
		*(uint16_t*)&lineCommand[1]											= linesToSend[iLine];
		{
			::gpk::mutex_guard														lock					(app.LockRender);
			*(::gpk::SCoord2<uint16_t>*)&lineCommand[3]							= finalImage.metrics().Cast<uint16_t>();
			memcpy(&lineCommand[7], finalImage.View[linesToSend[iLine]].begin(), finalImage.metrics().x * sizeof(::gpk::SColorBGRA));
		}
		{
			::gpk::mutex_guard														lock					(app.Server.UDPServer.Mutex);
			for(uint32_t iClient = 0, countClients = app.Server.UDPServer.Clients.size(); iClient < countClients; ++iClient) {
				int32_t clientSession = -1;
				for(uint32_t iSession = 0; iSession < app.Server.SessionMap.size(); ++iSession) {
					if(app.Server.SessionMap[iSession].IdConnection == (int32_t)iClient) {
						clientSession = (int32_t)iSession;
						break;
					}
				}
				if(-1 == clientSession)
					continue;
					
				if(app.Server.Server.Clients[clientSession].Stage != ::dwh::SESSION_STAGE_SERVER_ACCEPT_CLIENT)
					continue;
				if(0 == app.Server.UDPServer.Clients[iClient])
					continue;
				::gpk::SUDPConnection													& connection				= *app.Server.UDPServer.Clients[iClient];
				::gpk::connectionPushData(connection, connection.Queue, lineCommand);
			}
		}
		//::gpk::sleep(0);
	}

	//{
	//	::gpk::mutex_guard														lock					(app.LockRender);
	//	memcpy(app.DesktopImagePrevious.Texels.begin(), app.DesktopImage.Texels.begin(), app.DesktopImage.View.metrics().x * app.DesktopImage.View.metrics().y * sizeof(::gpk::SColorBGRA));
	//}
	app.RemoteInput.KeyboardPrevious									= app.RemoteInput.KeyboardCurrent;
	app.RemoteInput.MousePrevious										= app.RemoteInput.MouseCurrent;
	{
		::gpk::mutex_guard														lock					(app.Server.UDPServer.Mutex);
		for(uint32_t iClient = 0, countClients = app.Server.UDPServer.Clients.size(); iClient < countClients; ++iClient) {
			if(0 == app.Server.UDPServer.Clients[iClient])
				continue;
			::gpk::SUDPConnection													& currentConn			= *app.Server.UDPServer.Clients[iClient];
			if(::gpk::UDP_CONNECTION_STATE_IDLE != currentConn.State)
				continue;
			{
				::gpk::mutex_guard														lockQueue				(currentConn.Queue.MutexReceive);
				for(uint32_t iRecv = 0; iRecv < currentConn.Queue.Received.size(); ++iRecv) {
					if(0 == currentConn.Queue.Received[iRecv])
						continue;
					::gpk::SUDPConnectionMessage											& curMessage			= *currentConn.Queue.Received[iRecv];
					if(curMessage.Payload[0] != 7)
						continue;
					else {
						//info_printf("Received message from control client: %u.", (uint32_t)iClient);
						if(curMessage.Payload[1] == 1) {
							//info_printf("Keyboard input received.");
							app.RemoteInput.KeyboardCurrent = *(::gpk::SInputKeyboard*)&curMessage.Payload[2];
						}
						if(curMessage.Payload[1] == 1) {
							if(curMessage.Payload[2 + sizeof(::gpk::SInputKeyboard)] == 1) {
								app.RemoteInput.MouseCurrent = *(::gpk::SInputMouse*)&curMessage.Payload[2 + sizeof(::gpk::SInputKeyboard) + 1];
								//info_printf("Mouse input received. Mouse pos: {%u, %u}. Time: %llu.", app.RemoteInput.MouseCurrent.Position.x, app.RemoteInput.MouseCurrent.Position.y, ::gpk::timeCurrentInMs());
							}
						}
						else {
							if(curMessage.Payload[2] == 1) {
								app.RemoteInput.MouseCurrent = *(::gpk::SInputMouse*)&curMessage.Payload[3];
								//info_printf("Mouse input received (no Keyboard update). Mouse pos: {%u, %u}. Time: %llu.", app.RemoteInput.MouseCurrent.Position.x, app.RemoteInput.MouseCurrent.Position.y, ::gpk::timeCurrentInMs());
							}
						}
						currentConn.Queue.Received[iRecv] = {};
					}
				}
			}

		}
	}
	::gpk::SCoord2<int32_t> deltas = app.RemoteInput.MouseCurrent.Position - app.RemoteInput.MousePrevious.Position;
	app.RemoteInput.MouseCurrent.Deltas = {deltas.x, deltas.y, app.RemoteInput.MouseCurrent.Deltas.z};
	{
		::gpk::mutex_guard														lock						(app.LockGUI);
		//::gpk::guiProcessInput(gui, *app.Framework.Input);
		::gpk::guiProcessInput(gui, app.RemoteInput);
	}
	if(app.RemoteInput.MouseCurrent.Deltas.z) {
		//gui.Zoom.ZoomLevel													+= app.Framework.Input->MouseCurrent.Deltas.z * (1.0 / (120 * 4));
		gui.Zoom.ZoomLevel													+= app.RemoteInput.MouseCurrent.Deltas.z * (1.0 / (120 * 4));
		::gpk::guiUpdateMetrics(gui, app.Offscreen->Color.metrics(), true);
	}
 
	//SendInput();
	INPUT			inputs[256]	= {};
	uint32_t		totalInputs	= 0;
	for(uint32_t iKey = 1, keyCount = ::gpk::size(app.RemoteInput.KeyboardCurrent.KeyState); iKey < keyCount; ++iKey) {
		if(app.RemoteInput.KeyDown((uint8_t)iKey)) {
			info_printf("Key down (%u) '%c'", iKey, (char)iKey);
			inputs[totalInputs].type			= INPUT_KEYBOARD;
			inputs[totalInputs].ki.wVk			= 0;
			inputs[totalInputs].ki.wScan		= (uint8_t)iKey;
			inputs[totalInputs].ki.dwFlags		= 0;
			inputs[totalInputs].ki.time			= 0;
			inputs[totalInputs].ki.dwExtraInfo	= 0;
			++totalInputs;
		}
		else if(app.RemoteInput.KeyUp((uint8_t)iKey)) {
			info_printf("Key up (%u) '%c'", iKey, (char)iKey);
			inputs[totalInputs].type			= INPUT_KEYBOARD;
			inputs[totalInputs].ki.wVk			= 0;
			inputs[totalInputs].ki.wScan		= (uint8_t)iKey;
			inputs[totalInputs].ki.dwFlags		= KEYEVENTF_KEYUP;
			inputs[totalInputs].ki.time			= 0;
			inputs[totalInputs].ki.dwExtraInfo	= 0;
			++totalInputs;
		}
	}
	error_if(totalInputs && 0 == ::SendInput(totalInputs, inputs, sizeof(INPUT)), "Failed to send inputs to Windows.");
	totalInputs	= 0;
	if(app.RemoteInput.ButtonDown(0)) {
		info_printf("Button down (0)");
		inputs[totalInputs].type			= INPUT_MOUSE;
		inputs[totalInputs].mi.dwFlags		= MOUSEEVENTF_LEFTDOWN;
		inputs[totalInputs].mi.time			= 0;
		inputs[totalInputs].mi.dwExtraInfo	= 0;
		++totalInputs;
	}
	else if(app.RemoteInput.ButtonUp(0)) {
		info_printf("Button up (0)");
		inputs[totalInputs].type			= INPUT_MOUSE;
		inputs[totalInputs].mi.dwFlags		= MOUSEEVENTF_LEFTUP;
		inputs[totalInputs].mi.time			= 0;
		inputs[totalInputs].mi.dwExtraInfo	= 0;
		++totalInputs;
	}

	if(app.RemoteInput.ButtonDown(1)) {
		info_printf("Button down (1)");
		inputs[totalInputs].type			= INPUT_MOUSE;
		inputs[totalInputs].mi.dwFlags		= MOUSEEVENTF_RIGHTDOWN;
		inputs[totalInputs].mi.time			= 0;
		inputs[totalInputs].mi.dwExtraInfo	= 0;
		++totalInputs;
	}
	else if(app.RemoteInput.ButtonUp(1)) {
		info_printf("Button up (1)");
		inputs[totalInputs].type			= INPUT_MOUSE;
		inputs[totalInputs].mi.dwFlags		= MOUSEEVENTF_RIGHTUP;
		inputs[totalInputs].mi.time			= 0;
		inputs[totalInputs].mi.dwExtraInfo	= 0;
		++totalInputs;
	}

	if(app.RemoteInput.ButtonDown(2)) {
		info_printf("Button down (2)");
		inputs[totalInputs].type			= INPUT_MOUSE;
		inputs[totalInputs].mi.dwFlags		= MOUSEEVENTF_MIDDLEDOWN;
		inputs[totalInputs].mi.time			= 0;
		inputs[totalInputs].mi.dwExtraInfo	= 0;
		++totalInputs;
	}
	else if(app.RemoteInput.ButtonUp(2)) {
		info_printf("Button up (2)");
		inputs[totalInputs].type			= INPUT_MOUSE;
		inputs[totalInputs].mi.dwFlags		= MOUSEEVENTF_MIDDLEUP;
		inputs[totalInputs].mi.time			= 0;
		inputs[totalInputs].mi.dwExtraInfo	= 0;
		++totalInputs;
	}

	error_if(totalInputs && 0 == ::SendInput(totalInputs, inputs, sizeof(INPUT)), "Failed to send inputs to Windows.");

	::dwh::sessionServerUpdate(app.Server);

	::gpk::sleep(5);

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
