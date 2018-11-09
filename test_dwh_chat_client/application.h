#include "dwh_session_udp.h"

#include "gpk_framework.h"
#include "gpk_gui.h"

#include <mutex>

#ifndef APPLICATION_H_2078934982734
#define APPLICATION_H_2078934982734

namespace gme // I'm gonna use a different namespace in order to test a few things about the macros.
{
	enum MENU_LOGIN 
		{ MENU_LOGIN_USERNAME
		, MENU_LOGIN_PASSWORD
		};

	struct SApplication {
		::gpk::SFramework														Framework;
		::gpk::ptr_obj<::gpk::SRenderTarget<::gpk::SColorBGRA, uint32_t>>		Offscreen							= {};

		::dwh::SUDPSessionClient												Client								= {};

		int32_t																	IdExit								= -1;

		::gpk::ptr_obj<::gpk::SRenderTarget<::gpk::SColorBGRA, uint32_t>>		OffscreenRemote						= {};

		::std::mutex															LockGUI;
		::std::mutex															LockRender;

																				SApplication		(::gpk::SRuntimeValues& runtimeValues)	: Framework(runtimeValues)		{}
	};
} // namespace

#endif // APPLICATION_H_2078934982734
