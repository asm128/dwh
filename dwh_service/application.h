#include "gpk_udp_server.h"
#include "gpk_udp_client.h"
#include "dwh_session.h"

#include "gpk_framework.h"
#include "gpk_gui.h"

#include <mutex>

#ifndef APPLICATION_H_2078934982734
#define APPLICATION_H_2078934982734

namespace gme // I'm gonna use a different namespace in order to test a few things about the macros.
{
	struct SUDPSessionMap {
		int32_t																	IdConnection;
		int32_t																	IdSession	;
	};

	struct SApplication {
		::gpk::SFramework														Framework;
		::gpk::ptr_obj<::gpk::SRenderTarget<::gpk::SColorBGRA, uint32_t>>		Offscreen							= {};
		::gpk::SUDPServer														UDPServer							= {};
		::gpk::SUDPClient														UDPClient							= {};

		::dwh::SSessionServer													SessionServer;

		::gpk::array_pod<SUDPSessionMap>										SessionMap;

		int32_t																	IdExit								= -1;

		::std::mutex															LockGUI;
		::std::mutex															LockRender;

																				SApplication		(::gpk::SRuntimeValues& runtimeValues)	: Framework(runtimeValues)		{}
	};
} // namespace

#endif // APPLICATION_H_2078934982734
