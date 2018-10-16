#include "dwh_client_exports.h"
#include "dwh_client_session.h"
#include "gpk_udp_client.h"

#ifndef DWH_CLIENT_H_20181013
#define DWH_CLIENT_H_20181013

namespace dwh 
{
	struct SDWHClientManager {
							::gpk::array_obj<::gpk::SUDPClient>			Clients;
							::gpk::array_obj<::dwh::SDWHSession>		Sessions;
	};
} // namespace 

#endif // DWH_CLIENT_H_20181013
