#include "gpk_udp_client.h"
#include "gpk_udp_server.h"
#include "dwh_session.h"

#ifndef DWH_SESSION_UDP_H_293748923498
#define DWH_SESSION_UDP_H_293748923498

namespace dwh
{
	struct SUDPSessionAuthority { ::dwh::SSessionAuthority	Authority	= {}; ::gpk::SUDPServer		UDPServer	= {}; };
	struct SUDPSessionClient	{ ::dwh::SSessionClient		Client		= {}; ::gpk::SUDPClient		UDPClient	= {}; };
	struct SUDPSessionServer	{ ::dwh::SSessionServer		Server		= {}; ::gpk::SUDPServer		UDPServer	= {}; ::gpk::SUDPClient UDPClient = {}; };

	::gpk::error_t					authorityUpdate					(::gpk::SUDPServer& udpServer, ::dwh::SSessionAuthority & authority);
} // namespace 

#endif // DWH_SESSION_UDP_H_293748923498
