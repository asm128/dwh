#include "dwh_session_udp.h"

			::gpk::error_t											dwh::authorityUpdate		(::gpk::SUDPServer& udpServer, ::dwh::SSessionAuthority & authority)	{
	::gpk::mutex_guard														lock						(udpServer.Mutex);
	//SESSION_STAGE_CLIENT_CLOSED					= 0	// sessionClose							()
	//SESSION_STAGE_CLIENT_IDENTIFY						// authorityClientIdentifyRequest		() // Client	-> Authority	// Processed by authority
	//SESSION_STAGE_AUTHORITY_IDENTIFY					// authorityServerIdentifyResponse		() // Authority	-> Client		// Processed by client
	//SESSION_STAGE_CLIENT_REQUEST_SERVICE_START		// sessionClientStart					() // Client	-> Service		// Processed by service
	//SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST		// authorityServiceConfirmClientRequest	() // Service	-> Authority	// Processed by authority
	//SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT			// authorityServerConfirmClientResponse	() // Authority	-> Service		// Processed by service
	//SESSION_STAGE_SERVER_ACCEPT_CLIENT				// sessionServerAccept					() // Service	-> Client		// Processed by client
	//SESSION_STAGE_CLIENT_IDLE							// sessionClientAccepted				() // Client	-> IDLE			// Processed by client/service
	::gpk::array_pod<byte_t>												response;
	for(uint32_t iClient = 0; iClient < udpServer.Clients.size(); ++iClient) {
		::gpk::ptr_nco<::gpk::SUDPConnection>									pclient						= udpServer.Clients[iClient];
		if(0 == pclient || pclient->Socket == INVALID_SOCKET || pclient->State == ::gpk::UDP_CONNECTION_STATE_DISCONNECTED)
			continue;
		::gpk::SUDPConnection													& client					= *pclient;
		::gpk::array_obj<::gpk::ptr_obj<::gpk::SUDPConnectionMessage>>			received;
		{
			::gpk::mutex_guard														lockRecv					(client.Queue.MutexReceive);
			received															= client.Queue.Received;
			client.Queue.Received.clear();
		}
		for(uint32_t iMessage = 0; iMessage < received.size(); ++iMessage) {
			::gpk::ptr_nco<::gpk::SUDPConnectionMessage>							pmsg						= received[iMessage];
			if(0 == pmsg || 0 == pmsg->Payload.size())
				continue;
			::gpk::SUDPConnectionMessage											& msg						= *pmsg;
			response.clear();
			::dwh::SSessionCommand													command						= *(::dwh::SSessionCommand*)&msg.Payload[0];
			info_printf("Received command: %u.", command.Command);
			switch(command.Command) {																			  
			case ::dwh::SESSION_STAGE_CLIENT_IDENTIFY					: 
				ce_if(errored(::dwh::authorityServerIdentifyResponse		(authority, msg.Payload, response)), "Failed to process client command: %u.", (uint32_t)msg.Payload[0]); 
				ce_if(errored(::gpk::connectionPushData(client, client.Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command.Command); 
				break;
			case ::dwh::SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST	: 
				ce_if(errored(::dwh::authorityServerConfirmClientResponse	(authority, msg.Payload, response)), "Failed to process server command: %u.", (uint32_t)msg.Payload[0]); 
				ce_if(errored(::gpk::connectionPushData(client, client.Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command.Command); 
				break;
			default: 
				error_printf("Unrecognized session command: %u.", (uint32_t)command.Command);
				break;
			}
		}
	}
	return 0;
}
