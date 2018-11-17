#include "gpk_udp_server.h"
#include "gpk_udp_client.h"
#include "dwh_session_udp.h"

#include "gpk_framework.h"
#include "gpk_gui.h"

#include <mutex>

#ifndef APPLICATION_H_2078934982734
#define APPLICATION_H_2078934982734

	struct SOffscreenPlatformDetail { // raii destruction of resources
#if defined(GPK_WINDOWS)
		uint32_t																BitmapInfoSize						= 0;
		::BITMAPINFO															* BitmapInfo						= 0;
		::HDC																	IntermediateDeviceContext			= 0;    // <- note, we're creating, so it needs to be destroyed
		::HBITMAP																IntermediateBitmap					= 0;

																				~SOffscreenPlatformDetail			()																					{
			if(BitmapInfo					) ::free			(BitmapInfo					); 
			if(IntermediateBitmap			) ::DeleteObject	(IntermediateBitmap			); 
			if(IntermediateDeviceContext	) ::DeleteDC		(IntermediateDeviceContext	); 
		}
#endif
	};

namespace gme // I'm gonna use a different namespace in order to test a few things about the macros.
{
	struct SApplication {
		::gpk::SFramework														Framework;
		::gpk::ptr_obj<::gpk::SRenderTarget<::gpk::SColorBGRA, uint32_t>>		Offscreen							= {};

		::dwh::SUDPSessionServer												Server								= {};

		int32_t																	IdExit								= -1;

		::gpk::SInput															RemoteInput;

		::std::mutex															LockGUI;
		::std::mutex															LockRender;

		::gpk::SImage<::gpk::SColorBGRA>										DesktopImage;
		::gpk::SImage<::gpk::SColorBGRA>										DesktopImagePrevious;

		::gpk::SImage<uint16_t>													DesktopImage16;
		::gpk::SImage<uint16_t>													DesktopImage16Previous;

		::gpk::SImage<uint8_t>													DesktopImageB;
		::gpk::SImage<uint8_t>													DesktopImageBPrevious;
		::gpk::SImage<uint8_t>													DesktopImageG;
		::gpk::SImage<uint8_t>													DesktopImageGPrevious;
		::gpk::SImage<uint8_t>													DesktopImageR;
		::gpk::SImage<uint8_t>													DesktopImageRPrevious;


		bool																	Source16Bit							= true;
		bool																	Compress							= true;
		bool																	ChannelWise							= true;

		SOffscreenPlatformDetail												OffscreenDetail;

																				SApplication		(::gpk::SRuntimeValues& runtimeValues)	: Framework(runtimeValues)		{}
	};
} // namespace

#endif // APPLICATION_H_2078934982734
