#include "dwh_storage_server.h"

	::gpk::error_t										dwh::serverStop									(::dwh::SDWHServerStorage& instance)					{ instance; return 0; }
	::gpk::error_t										dwh::serverStart								(::dwh::SDWHServerStorage& instance, uint16_t port)		{ ::gpk::serverStart(instance.NetworkServer, port); return 0; }
	::gpk::error_t										dwh::serverUpdate								(::dwh::SDWHServerStorage& instance)					{ 
	for(uint32_t iClient = 0; iClient < instance.NetworkServer.Clients.size(); ++iClient) {
		::gpk::ptr_nco<::gpk::SUDPConnection>						conn;
		{
			::gpk::mutex_guard											lock											(instance.NetworkServer.Mutex);
			conn													= instance.NetworkServer.Clients[iClient];
		}
		if(0 == conn || conn->State != ::gpk::UDP_CONNECTION_STATE_IDLE)
			continue;
		{
			::gpk::mutex_guard											lock											(conn->Queue.MutexReceive);
			for(uint32_t iReceived = 0; iReceived < conn->Queue.Received.size(); ++iReceived) {
				if(0 == conn->Queue.Received[iReceived])
					continue;
				if(conn->Queue.Received[iReceived]->Command.Command != ::gpk::ENDPOINT_COMMAND_PAYLOAD)
					continue;
			}
		}
	}
	return 0; 
}

