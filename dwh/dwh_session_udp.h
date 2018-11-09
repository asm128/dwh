#include "gpk_udp_client.h"
#include "gpk_udp_server.h"
#include "dwh_session.h"

#ifndef DWH_SESSION_UDP_H_293748923498
#define DWH_SESSION_UDP_H_293748923498

namespace dwh
{
	struct SUDPSessionAuthority { ::dwh::SSessionAuthority	Authority	= {}; ::gpk::SUDPServer		UDPServer	= {}; };
	struct SUDPSessionServer	{ ::dwh::SSessionServer		Server		= {}; ::gpk::SUDPServer		UDPServer	= {}; ::gpk::SUDPClient UDPClient = {}; 
		struct SUDPSessionMap {
			int32_t								IdConnection;
			int32_t								IdSession	;
		};
		::gpk::array_pod<SUDPSessionMap>	SessionMap;
	};
	struct SUDPSessionClient	{ ::dwh::SSessionClient		Client		= {}; ::gpk::SUDPClient		UDPClient	= {}; 
		::gpk::SIPv4							AddressServer					= {}; 
		::gpk::SIPv4							AddressAuthority				= {}; 
	};

	::gpk::error_t					sessionAuthorityUpdate			(::dwh::SUDPSessionAuthority	& authority);
	::gpk::error_t					sessionServerUpdate				(::dwh::SUDPSessionServer		& server);
	::gpk::error_t					sessionClientConnect			(::dwh::SUDPSessionClient		& client);
	
	//::gpk::error_t					sessionServerSendSecure			(::dwh::SUDPSessionClient		& client);
	//::gpk::error_t					sessionServerSendFast			(::dwh::SUDPSessionClient		& client);
	//::gpk::error_t					sessionServerRecv				(::dwh::SUDPSessionClient		& client);
	//::gpk::error_t					sessionClientSendSecure			(::dwh::SUDPSessionClient		& client);
	//::gpk::error_t					sessionClientSendFast			(::dwh::SUDPSessionClient		& client);
	//::gpk::error_t					sessionClientRecv				(::dwh::SUDPSessionClient		& client);
} // namespace 

#endif // DWH_SESSION_UDP_H_293748923498
