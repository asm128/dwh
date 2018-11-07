#include "dwh_session_udp.h"
#include "gpk_timer.h"

			::gpk::error_t											dwh::sessionAuthorityUpdate		(::dwh::SUDPSessionAuthority	& session)	{
	::gpk::SUDPServer														& udpServer						= session.UDPServer; 
	::dwh::SSessionAuthority												& authority						= session.Authority;

	::gpk::mutex_guard														lock							(udpServer.Mutex);
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


	//SESSION_STAGE_CLIENT_CLOSED					= 0	// sessionClose							()
	//SESSION_STAGE_CLIENT_IDENTIFY						// authorityClientIdentifyRequest		() // Client	-> Authority	// Processed by authority
	//SESSION_STAGE_AUTHORITY_IDENTIFY					// authorityServerIdentifyResponse		() // Authority	-> Client		// Processed by client
	//SESSION_STAGE_CLIENT_REQUEST_SERVICE_START		// sessionClientStart					() // Client	-> Service		// Processed by service
	//SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST		// authorityServiceConfirmClientRequest	() // Service	-> Authority	// Processed by authority
	//SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT			// authorityServerConfirmClientResponse	() // Authority	-> Service		// Processed by service
	//SESSION_STAGE_SERVER_ACCEPT_CLIENT				// sessionServerAccept					() // Service	-> Client		// Processed by client
	//SESSION_STAGE_CLIENT_IDLE							// sessionClientAccepted				() // Client	-> IDLE			// Processed by client/service
			::gpk::error_t											dwh::sessionClientConnect		(::dwh::SUDPSessionClient & client)										{
	uint64_t																totalTime						= 0;
	::gpk::STimer															timer;
	client.UDPClient.AddressConnect										= client.AddressAuthority;
	ree_if(errored(::gpk::clientConnect(client.UDPClient)), "Failed to connect to authority server at: %u.%u.%u.%u:%u.", GPK_IPV4_EXPAND(client.UDPClient.AddressConnect));

	::gpk::array_pod<byte_t>												dataToSend;
	::dwh::authorityClientIdentifyRequest	(client.Client, dataToSend);
	::gpk::connectionPushData(client.UDPClient, client.UDPClient.Queue, dataToSend);
	::gpk::array_pod<byte_t>												dataReceived;
	while(true) {
		::gpk::clientUpdate(client.UDPClient);
		break_warn_if(INVALID_SOCKET == client.UDPClient.Socket || ::gpk::UDP_CONNECTION_STATE_DISCONNECTED == client.UDPClient.State, "Client closed.");
		::gpk::mutex_guard														lock_rcv					(client.UDPClient.Queue.MutexReceive);
		for(uint32_t iRcv = 0, countRcv = client.UDPClient.Queue.Received.size(); iRcv < countRcv; ++iRcv) {
			::gpk::ptr_obj<::gpk::SUDPConnectionMessage>							message						= client.UDPClient.Queue.Received[iRcv];
			if(0 == message || 0 == message->Payload.size())
				continue;
			::dwh::SSessionCommand													& command					= *(::dwh::SSessionCommand*)message->Payload.begin();
			switch(command.Command) {
			case ::dwh::SESSION_STAGE_AUTHORITY_IDENTIFY	:	
				info_printf("%s", "Received identify response.");
				dataReceived														= message->Payload;
				break;
			default:
				error_printf("Invalid session command received by client: %u.", (uint32_t)command.Command);
			}
		}
		if(0 != dataReceived.size())
			break;
	}
	::gpk::clientDisconnect(client.UDPClient);
	timer.Frame();
	totalTime																+= timer.LastTimeMicroseconds;
	double																		inSeconds					= totalTime / 1000.0;
	info_printf("---- Public key retrieved in %g milliseconds.", inSeconds);

	dataToSend.clear();
	gpk_necall(::dwh::sessionClientStart(client.Client, dataReceived, dataToSend), "%s", "Failed to start session.");

	client.UDPClient.AddressConnect										= client.AddressServer;
	ree_if(errored(::gpk::clientConnect(client.UDPClient)), "Failed to connect to service's server at: %u.%u.%u.%u:%u.", GPK_IPV4_EXPAND(client.UDPClient.AddressConnect));
	::gpk::connectionPushData(client.UDPClient, client.UDPClient.Queue, dataToSend);
	dataReceived.clear();
	while(true) {
		::gpk::clientUpdate(client.UDPClient);
		break_warn_if(INVALID_SOCKET == client.UDPClient.Socket || ::gpk::UDP_CONNECTION_STATE_DISCONNECTED == client.UDPClient.State, "Client closed.");
		for(uint32_t iRcv = 0, countRcv = client.UDPClient.Queue.Received.size(); iRcv < countRcv; ++iRcv) {
			::gpk::ptr_obj<::gpk::SUDPConnectionMessage>							message						= client.UDPClient.Queue.Received[iRcv];
			if(0 == message || 0 == message->Payload.size())
				continue;
			::dwh::SSessionCommand													& command					= *(::dwh::SSessionCommand*)message->Payload.begin();
			switch(command.Command) {
			case ::dwh::SESSION_STAGE_SERVER_ACCEPT_CLIENT	:	
				info_printf("%s", "Received accept response.");
				dataReceived														= message->Payload;
				break;
			//default:
			//	error_printf("Invalid session command received by client: %u.", (uint32_t)command.Command);
			}
		}
		if(0 != dataReceived.size())
			break;
	}
	dataToSend.clear();
	::dwh::sessionClientAccepted(client.Client, dataReceived, dataToSend);
	timer.Frame();
	totalTime																+= timer.LastTimeMicroseconds;
	inSeconds																= totalTime / 1000.0;
	info_printf("---- Session keys set in %g milliseconds.", inSeconds);
	info_printf("RSA Private  : %llu.", client.Client.RSAKeys.Private  );
	info_printf("RSA PrivateN : %llu.", client.Client.RSAKeys.PrivateN );
	info_printf("RSA Public   : %llu.", client.Client.RSAKeys.Public   );
	info_printf("RSA PublicN  : %llu.", client.Client.RSAKeys.PublicN  );
	info_printf("Ardell       : %llx.", client.Client.KeySymmetric[0]  );
	for(uint32_t i=1; i < 5; ++i)
		info_printf("AES[%u]        : %llx.", i - 1, client.Client.KeySymmetric[i]);
	return 0;
}


			::gpk::error_t											dwh::sessionServerUpdate				(::dwh::SUDPSessionServer & session)	{ 
	::gpk::array_pod<byte_t>												response;
	{
		::gpk::mutex_guard														lock						(session.UDPServer.Mutex);
		//SESSION_STAGE_CLIENT_CLOSED					= 0	// sessionClose							()
		//SESSION_STAGE_CLIENT_IDENTIFY						// authorityClientIdentifyRequest		() // Client	-> Authority	// Processed by authority
		//SESSION_STAGE_AUTHORITY_IDENTIFY					// authorityServerIdentifyResponse		() // Authority	-> Client		// Processed by client
		//SESSION_STAGE_CLIENT_REQUEST_SERVICE_START		// sessionClientStart					() // Client	-> Service		// Processed by service
		//SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST		// authorityServiceConfirmClientRequest	() // Service	-> Authority	// Processed by authority
		//SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT			// authorityServerConfirmClientResponse	() // Authority	-> Service		// Processed by service
		//SESSION_STAGE_SERVER_ACCEPT_CLIENT				// sessionServerAccept					() // Service	-> Client		// Processed by client
		//SESSION_STAGE_CLIENT_IDLE							// sessionClientAccepted				() // Client	-> IDLE			// Processed by client/service
		for(uint32_t iClient = 0; iClient < session.UDPServer.Clients.size(); ++iClient) {
			::gpk::ptr_nco<::gpk::SUDPConnection>									pclient						= session.UDPServer.Clients[iClient];
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
				//info_printf("Received: %.1024s.", msg.Payload.begin());
				response.clear();
				::dwh::SSessionCommand													command						= *(::dwh::SSessionCommand*)&msg.Payload[0];
				switch(command.Command) {
				case ::dwh::SESSION_STAGE_CLIENT_REQUEST_SERVICE_START	: 
					{
						int32_t																indexClient					= -1;
						ce_if(errored(indexClient = ::dwh::authorityServiceConfirmClientRequest(session.Server, msg.Payload, response)), "Failed to process client command: %u.", (uint32_t)command.Command);
						session.SessionMap.push_back({(int32_t)iClient, indexClient});
						::gpk::clientDisconnect(session.UDPClient);
						::gpk::tcpipAddress(32766, 0, ::gpk::TRANSPORT_PROTOCOL_UDP, session.UDPClient.AddressConnect);
						ree_if(errored(::gpk::clientConnect(session.UDPClient)), "Failed to connect to authority server at: %u.%u.%u.%u:%u.", GPK_IPV4_EXPAND(session.UDPClient.AddressConnect));
						ce_if(errored(::gpk::connectionPushData(session.UDPClient, session.UDPClient.Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command.Command);
					}
					break;
				//case ::dwh::SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT		: 
				//	ce_if(errored(::dwh::sessionServerAccept(app.SessionServer, msg.Payload, response)), "Failed to process server command: %u.", (uint32_t)msg.Payload[0]); 
				//	ce_if(errored(::gpk::connectionPushData(client, client.Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command); break;
				default: 
					error_printf("Unrecognized session command: %u.", command.Command);
					break;
				}
			}
		}
	}

	if(session.UDPClient.State == ::gpk::UDP_CONNECTION_STATE_IDLE)
		::gpk::clientUpdate(session.UDPClient);

	::gpk::SUDPConnection													& client					= session.UDPClient;
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
		info_printf("Received: %s.", msg.Payload.begin());
		response.clear();
		::dwh::SSessionCommand													command						= *(::dwh::SSessionCommand*)&msg.Payload[0];
		int32_t																	indexClientAccepted			= -1;
		switch(command.Command) {
		//case ::dwh::SESSION_STAGE_CLIENT_REQUEST_SERVICE_START	: 
		//	ce_if(errored(::dwh::authorityServiceConfirmClientRequest	(app.SessionServer, msg.Payload, response)), "Failed to process client command: %u.", (uint32_t)msg.Payload[0]); 
		//	ce_if(errored(::gpk::connectionPushData						(app.UDPClient, app.UDPClient.Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command); 
		//	break;
		case ::dwh::SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT		: 
			ce_if(errored(indexClientAccepted = ::dwh::sessionServerAccept(session.Server, msg.Payload, response)), "Failed to process server command: %u.", (uint32_t)command.Command); 
			ce_if(session.Server.Clients.size() <= (uint32_t)indexClientAccepted, "Invalid client index: %i.", indexClientAccepted);
			indexClientAccepted = session.SessionMap[indexClientAccepted].IdConnection;
			ce_if(errored(::gpk::connectionPushData(*session.UDPServer.Clients[indexClientAccepted], session.UDPServer.Clients[indexClientAccepted]->Queue, response)), "Failed to push response data for command: %u.", (uint32_t)command.Command); 
			break;
		default: 
			error_printf("Unrecognized session command: %u.", (uint32_t)command.Command);
			break;
		}
	}
	return 0;
}
