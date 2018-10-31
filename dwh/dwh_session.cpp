#include "dwh_session.h"
#include "gpk_rsa.h"
#include "gpk_chrono.h"
#include "gpk_noise.h"

static constexpr	const uint32_t				SIZE_MAX_CERTIFICATE										= 2040;
static constexpr	const uint32_t				SIZE_MAX_KEY												= 256;
static constexpr	const uint32_t				SIZE_MAX_CLIENT_ID											= 256;

#pragma pack(push, 1)
struct SAuthorityClientIdentifyRequest			{
						::dwh::SESSION_STAGE		Command														= ::dwh::SESSION_STAGE_CLIENT_CLOSED;
						uint64_t					IdServer													= (uint64_t)-1LL;
						uint64_t					Certificate			[SIZE_MAX_CERTIFICATE]					= {};
};

struct SAuthorityServerIdentifyResponse			{
						::dwh::SESSION_STAGE		Command														= ::dwh::SESSION_STAGE_CLIENT_CLOSED;
						::dwh::SKeyPair				Keys														= {};
						uint64_t					IdClient			[SIZE_MAX_CLIENT_ID]					= {};
};

struct SSessionClientStart						{
						::dwh::SESSION_STAGE		Command														= ::dwh::SESSION_STAGE_CLIENT_CLOSED;
						uint64_t					IdClient			[SIZE_MAX_CLIENT_ID]					= {};
};

struct SAuthorityServiceConfirmClientRequest	{
						::dwh::SESSION_STAGE		Command														= ::dwh::SESSION_STAGE_CLIENT_CLOSED;
						uint64_t					IdClient			[SIZE_MAX_CLIENT_ID]					= {};
						uint64_t					IdServer													= {};
};

struct SAuthorityServerConfirmClientResponse	{
						::dwh::SESSION_STAGE		Command														= ::dwh::SESSION_STAGE_CLIENT_CLOSED;
						::dwh::SKeyPair				Keys														= {};
						uint64_t					IdClient			[SIZE_MAX_CLIENT_ID - 32]				= {};
};

struct SSessionServerAccept						{
						::dwh::SESSION_STAGE		Command														= ::dwh::SESSION_STAGE_CLIENT_CLOSED;
						uint64_t					KeysSymmetric		[SIZE_MAX_KEY]							= {};
};

//struct SSessionClientAccepted					{};
#pragma pack(pop)

// Client sends a request to the authority server and software identifier.																
					::gpk::error_t				dwh::authorityClientIdentifyRequest					(::dwh::SSessionClient		& client	, ::gpk::array_pod<byte_t> & output)												{ ;   client			, output; 
	client.Stage									= ::dwh::SESSION_STAGE_CLIENT_IDENTIFY;
	client.IdServer									= 0;
	client.IdClient									= (uint64_t)-1LL;
	client.RSAKeys									= {};

	::SAuthorityClientIdentifyRequest					dataToSend											= {};
	dataToSend.Command								= ::dwh::SESSION_STAGE_CLIENT_IDENTIFY;
	// Fill initial request with certificate and client information.
	for(uint32_t iByte = 0; iByte < ::gpk::size(dataToSend.Certificate); ++iByte)	
		dataToSend.Certificate[iByte]					= (ubyte_t)(::gpk::size(dataToSend.Certificate) - 1 - iByte);
	dataToSend.IdServer								= client.IdServer;
	gpk_necall(output.resize(sizeof(::SAuthorityClientIdentifyRequest)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(::SAuthorityClientIdentifyRequest));
	return 0;		 
}

// Authority's server sends a request with the public key for session startup.															
					::gpk::error_t				dwh::authorityServerIdentifyResponse				(::dwh::SSessionAuthority	& authority	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   authority	, input	, output; 
	if(0 == authority.Servers.size())
		authority.Servers.resize(1);

	const ::SAuthorityClientIdentifyRequest				& dataReceived										= *(::SAuthorityClientIdentifyRequest*)input.begin();

	bool												validCertificate									= true;
	bool												validServerId										= authority.Servers.size() > dataReceived.IdServer;
	uint64_t											idClient											= 0;
	::dwh::SKeyPair										keysClient											= {};
	if(false == validServerId) 
	{}
	else {
		// Evaluate certificate and client information.
		for(uint32_t iByte = 0; iByte < ::gpk::size(dataReceived.Certificate); ++iByte)
			if(dataReceived.Certificate[iByte] != (ubyte_t)(::gpk::size(dataReceived.Certificate) - 1 - iByte)) 
				validCertificate								= false;
	
		idClient										= validCertificate ? ::gpk::timeCurrentInUs() : (uint64_t)-1LL;
		if(validCertificate) { // Initialize a record for this client if the request is valid
			::dwh::SSessionClientRecord							newClient											= {};
			newClient.Stage									= ::dwh::SESSION_STAGE_AUTHORITY_IDENTIFY;
			newClient.IdClient								= idClient;


			uint64_t											clienttoservern										= 0;
			uint64_t											servertoclientn										= 0;
			uint64_t											clienttoserverpublic								= 0;
			uint64_t											servertoclientpublic								= 0;
			uint64_t											clienttoserverprivate								= 0;
			uint64_t											servertoclientprivate								= 0;

			{
				uint64_t											prime1Public										= 251; 
				uint64_t											prime2Public										= 241; 
				servertoclientn									= prime1Public * prime2Public;
				servertoclientpublic							= 277;
				servertoclientprivate							= 13213;
			}
			{
				uint64_t											prime1Private										= 19; 
				uint64_t											prime2Private										= 23; 
				clienttoservern									= prime1Private * prime2Private;
				clienttoserverpublic							= 337;
				clienttoserverprivate							= 349;
			}

			newClient.RSAKeysClient.Private					= servertoclientprivate;
			newClient.RSAKeysClient.PrivateN				= servertoclientn;
			newClient.RSAKeysClient.Public					= clienttoserverpublic;
			newClient.RSAKeysClient.PublicN					= clienttoservern;

			newClient.RSAKeysServer.Private				= clienttoserverprivate;
			newClient.RSAKeysServer.PrivateN				= clienttoservern;
			newClient.RSAKeysServer.Public					= servertoclientpublic;
			newClient.RSAKeysServer.PublicN				= servertoclientn;		

			gpk_necall(authority.Servers[(uint32_t)dataReceived.IdServer].Clients.push_back(newClient), "Out of memory?");

			keysClient										= newClient.RSAKeysClient;
		}
	}

	// Send keys to client.
	::SAuthorityServerIdentifyResponse					dataToSend											= {};
	dataToSend.Command								= ::dwh::SESSION_STAGE_AUTHORITY_IDENTIFY;
	dataToSend.Keys									= keysClient;
	if(false == validCertificate) {
		memset(dataToSend.IdClient, 0xFF, ::gpk::size(dataToSend.IdClient) * sizeof(dataToSend.IdClient[0]));
		dataToSend.Keys.Public							= (uint64_t)-1LL;
	}
	else if(false == validServerId) {
		memset(dataToSend.IdClient, 0xFF, ::gpk::size(dataToSend.IdClient) * sizeof(dataToSend.IdClient[0]));
		dataToSend.Keys.Public							= (uint64_t)-2LL;
	}
	else {
		dataToSend.IdClient[0]							= idClient;
	}
	gpk_necall(output.resize(sizeof(::SAuthorityServerIdentifyResponse)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(::SAuthorityServerIdentifyResponse));
	return 0; 
}	

// Client sends a request to the service's server.																						
					::gpk::error_t				dwh::sessionClientStart								(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   client	, input	, output; 
	client.Stage									= ::dwh::SESSION_STAGE_CLIENT_REQUEST_SERVICE_START;
	::SAuthorityServerIdentifyResponse					& dataReceived										= *(::SAuthorityServerIdentifyResponse*)input.begin();

	ree_if(errored(client.IdClient = dataReceived.IdClient[0]), "Client not authorized.");
	client.RSAKeys									= dataReceived.Keys;

	// Build service connection request encrypted with the public key received from the authority server.

	::SSessionClientStart								dataToSend											= {};
	dataToSend.Command								= ::dwh::SESSION_STAGE_CLIENT_REQUEST_SERVICE_START;
	dataToSend.IdClient[0]							= client.IdClient;
	gpk_necall(output.resize(sizeof(SSessionClientStart)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SSessionClientStart));
	return 0; 
}

// Service server sends a request to authority server with client information.															
					::gpk::error_t				dwh::authorityServiceConfirmClientRequest			(::dwh::SSessionServer		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   service	, input	, output; 
	::SSessionClientStart								dataReceived										= *(::SSessionClientStart*)input.begin();

	bool												invalidClient										= false;
	for(uint32_t iClient = 0, countClients = service.Clients.size(); iClient < countClients; ++iClient)
		if(service.Clients[iClient].IdClient == dataReceived.IdClient[0]) {
			invalidClient								= true;
			error_printf("Client already exists: %llu.", service.Clients[iClient].IdClient);
			memset(dataReceived.IdClient, 0xFF, ::gpk::size(dataReceived.IdClient) * sizeof(dataReceived.IdClient[0]));
		}

	if(false == invalidClient) {
		::dwh::SSessionClientRecord							newClient;
		newClient.Stage									= ::dwh::SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST;
		newClient.IdClient								= dataReceived.IdClient[0];
		service.Clients.push_back(newClient);
	}

	// Build request with client information in order to query the authority server for the private key.
	::SAuthorityServiceConfirmClientRequest				dataToSend											= {};
	dataToSend.Command								= ::dwh::SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST;
	dataToSend.IdClient[0]							= dataReceived.IdClient[0];
	gpk_necall(output.resize(sizeof(SAuthorityServiceConfirmClientRequest)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SAuthorityServiceConfirmClientRequest));
	return 0; 
}	

// Authority server sends a response to service with client information and the private key for decrypting the client password.			
					::gpk::error_t				dwh::authorityServerConfirmClientResponse			(::dwh::SSessionAuthority	& authority	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   authority	, input	, output; 
	::SAuthorityServiceConfirmClientRequest				dataReceived										= *(::SAuthorityServiceConfirmClientRequest*)input.begin();

	// Build response with private key information in order to send to the service and allow it to decrypt the data received from the client.
	bool												validClient											= false;
	::dwh::SKeyPair										keysServer											= {};
	{
		::dwh::SSessionServer								server												= authority.Servers[(uint32_t)dataReceived.IdServer];
		for(uint32_t iClient = 0, countClients = server.Clients.size(); iClient < countClients; ++iClient )
			if(server.Clients[iClient].IdClient == dataReceived.IdClient[0]) {
				keysServer										= server.Clients[iClient].RSAKeysServer;
				validClient										= true;
				server.Clients[iClient].Stage					= SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT;
			}
	}

	::SAuthorityServerConfirmClientResponse				dataToSend											= {};
	dataToSend.Command								= ::dwh::SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT;
	dataToSend.Keys									= keysServer;				
	dataToSend.IdClient[0]							= dataReceived.IdClient[0];
	gpk_necall(output.resize(sizeof(SAuthorityServerConfirmClientResponse)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SAuthorityServerConfirmClientResponse));
	return validClient ? 0 : 1; 
}	

// Service server sends a response to the client containing the symmetric keys for the rest of the communication.						
					::gpk::error_t				dwh::sessionServerAccept							(::dwh::SSessionServer		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   service	, input	, output; 
	::SAuthorityServerConfirmClientResponse				dataReceived										= *(::SAuthorityServerConfirmClientResponse*)input.begin();

	// Decrypt client data and build accept/reject response to be sent to the client.
	bool												accepted											= false;
	for(uint32_t iClient = 0, countClients = service.Clients.size(); iClient < countClients; ++iClient)
		if(service.Clients[iClient].IdClient == dataReceived.IdClient[0]) {
			accepted									= true;
			service.Clients[iClient].Stage				= ::dwh::SESSION_STAGE_SERVER_ACCEPT_CLIENT;
			service.Clients[iClient].RSAKeysServer		= dataReceived.Keys;
			break;
		}

	::SSessionServerAccept								dataToSend											= {};
	dataToSend.Command								= ::dwh::SESSION_STAGE_SERVER_ACCEPT_CLIENT;
	if(accepted) 
		dataToSend.KeysSymmetric[0]						= ::gpk::noise1DBase(::gpk::timeCurrentInUs());
	else
		memset(dataToSend.KeysSymmetric, 0xFF, ::gpk::size(dataToSend.KeysSymmetric) * sizeof(dataToSend.KeysSymmetric[0]));

	gpk_necall(output.resize(sizeof(SSessionServerAccept)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SSessionServerAccept));
	return 0; 
}	

// Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
					::gpk::error_t				dwh::sessionClientAccepted							(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   client	, input	, output; 
	::SSessionServerAccept								dataReceived										= *(::SSessionServerAccept*)input.begin();

	// Evaluate server response to determine if we can enter into IDLE state or not.
	ree_if(-1LL == (int64_t)(client.KeySymmetric = dataReceived.KeysSymmetric[0]),  "Service rejected communication.");
	client.Stage									= ::dwh::SESSION_STAGE_CLIENT_IDLE;
	//ree_if(client.KeySymmetric != 987654321ULL,  "Key test failed.");

	//::SSessionClientAccepted								dataToSend											= {};
	//gpk_necall(output.resize(sizeof(SSessionClientAccepted)), "Out of memory?");
	//memcpy(output.begin(), &dataToSend, sizeof(SSessionClientAccepted));
	return 0; 
}	

					::gpk::error_t				hash						(const ::gpk::view_array<const byte_t> & input, uint64_t & output) { 
	output											= 0;
	for(uint32_t i = 0; i < input.size() - 1; ++i) {
		const uint64_t									hashedChar					= ::gpk::noise1DBase((i * input[i] + ::gpk::noise1DBase(input[i + 1]), ::dwh::NOISE_SEED));
		output										+= hashedChar;
	}
	return input.size();
}

					::gpk::error_t				dwh::sessionHash			(::dwh::SSessionClient		& client	, const ::gpk::view_array<const byte_t> & input, uint64_t & output) { 
	client;
	return hash(input, output);							
}

#pragma pack(push, 1)
struct SSessionPayloadHeader {
	::dwh::SESSION_STAGE			Command		;
	uint64_t						Size		;
	uint64_t						IdClient	;
	uint64_t						Hash		;
};

struct SSessionPayloadFooter {
	uint64_t						Hash;
};
#pragma pack(pop)

					::gpk::error_t				dwh::sessionClientEncrypt							(::dwh::SSessionClient		& client	, const ::gpk::view_array<const byte_t> & input, ::gpk::array_pod<byte_t> & output) { 
	::gpk::array_pod<uint64_t>							encrypted;
	gpk_necall(::gpk::gpcEncodeWithHash(input, client.RSAKeys.PublicN, client.RSAKeys.Public, 0, true, encrypted), "%s", "Failed to encrypt message.");

	SSessionPayloadHeader								header												= {}; 
	header.Command									= ::dwh::SESSION_STAGE_CLIENT_IDLE;
	header.Size										= encrypted.size() * sizeof(uint64_t);
	header.IdClient									= client.IdClient;
	uint32_t											outputOffset										= output.size();
	output.resize(outputOffset + sizeof(SSessionPayloadHeader) + sizeof(SSessionPayloadFooter) + (uint32_t)header.Size);
	*(SSessionPayloadHeader*)&output[outputOffset]	= header;
	memcpy(&output[outputOffset + sizeof(SSessionPayloadHeader)], encrypted.begin(), header.Size);

	SSessionPayloadFooter								footer												= {};
	::hash({&output[outputOffset], (uint32_t)header.Size + sizeof(SSessionPayloadHeader)}, footer.Hash);
	*(SSessionPayloadFooter*)&output[outputOffset + (uint32_t)header.Size + sizeof(SSessionPayloadHeader)]	= footer;
	return 0; 
}	// 

					::gpk::error_t				getClientIndex										(::gpk::array_pod<::dwh::SSessionClientRecord> clients, uint64_t idClient) {
	for(uint32_t i = 0; i < clients.size(); ++i) 
		if(clients[i].IdClient == idClient)
			return i;
	return -1;
}

					::gpk::error_t				dwh::sessionServerDecrypt							(::dwh::SSessionServer		& server	, uint64_t idClient, const ::gpk::view_array<const byte_t> & input, ::gpk::array_pod<byte_t> & output) { server, input, output; 
	::SSessionPayloadHeader								& header											= *(::SSessionPayloadHeader*)input.begin(); 
	::SSessionPayloadFooter								& footer											= *(::SSessionPayloadFooter*)&input[input.size() - sizeof(::SSessionPayloadFooter)]; 
	uint64_t											hashCheck											= 0;
	::hash({input.begin(), input.size() - (uint32_t)sizeof(SSessionPayloadFooter)}, hashCheck);
	ree_if(footer.Hash != hashCheck, "Failed to check hash. footer.Hash: %u. hashCheck: %u.", footer.Hash, hashCheck);
	::gpk::array_pod<byte_t>							decrypted											= {};
	const int32_t										indexClient											= ::getClientIndex(server.Clients, idClient);
	gpk_necall(::gpk::gpcDecodeWithHash({(uint64_t*)&input[sizeof(::SSessionPayloadHeader)], (uint32_t)header.Size / sizeof(uint64_t)}, server.Clients[indexClient].RSAKeysServer.PrivateN, server.Clients[indexClient].RSAKeysServer.Private, true, decrypted), "Failed to decrypt session message.");
	uint32_t											outputOffset										= output.size();
	gpk_necall(output.resize(decrypted.size() + outputOffset), "%s", "Out of memory?");
	memcpy(&output[outputOffset], decrypted.begin(), decrypted.size());
	return 0; 
}	// 

					::gpk::error_t				dwh::sessionServerEncrypt							(::dwh::SSessionServer		& server	, uint64_t idClient, const ::gpk::view_array<const byte_t> & input, ::gpk::array_pod<byte_t> & output) { 
	::gpk::array_pod<uint64_t>							encrypted;
	const int32_t										indexClient											= ::getClientIndex(server.Clients, idClient);
	ree_if(errored(indexClient), "Invalid client id: %llu.", idClient);
	::dwh::SSessionClientRecord							& client											= server.Clients[indexClient];
	gpk_necall(::gpk::gpcEncodeWithHash(input, client.RSAKeysServer.PublicN, client.RSAKeysServer.Public, 0, true, encrypted), "%s", "Failed to encrypt message.");

	SSessionPayloadHeader								header												= {}; 
	header.Command									= ::dwh::SESSION_STAGE_CLIENT_IDLE;
	header.Size										= encrypted.size() * sizeof(uint64_t);
	header.IdClient									= client.IdClient;
	uint32_t											outputOffset										= output.size();
	gpk_necall(output.resize(outputOffset + sizeof(SSessionPayloadHeader) + sizeof(SSessionPayloadFooter) + (uint32_t)header.Size), "%s", "Out of memory?");
	*(SSessionPayloadHeader*)&output[outputOffset]	= header;
	memcpy(&output[outputOffset + sizeof(SSessionPayloadHeader)], encrypted.begin(), header.Size);

	SSessionPayloadFooter								footer												= {};
	::hash({&output[outputOffset], (uint32_t)header.Size + sizeof(SSessionPayloadHeader)}, footer.Hash);
	*(SSessionPayloadFooter*)&output[outputOffset + (uint32_t)header.Size + sizeof(SSessionPayloadHeader)]	= footer;
	return 0; 
}	// 

					::gpk::error_t				dwh::sessionClientDecrypt							(::dwh::SSessionClient		& client	, const ::gpk::view_array<const byte_t> & input, ::gpk::array_pod<byte_t> & output) { client, input, output; 
	::SSessionPayloadHeader								& header											= *(::SSessionPayloadHeader*)input.begin(); 
	::SSessionPayloadFooter								& footer											= *(::SSessionPayloadFooter*)&input[input.size() - sizeof(::SSessionPayloadFooter)]; 
	uint64_t											hashCheck											= 0;
	::hash({input.begin(), input.size() - (uint32_t)sizeof(SSessionPayloadFooter)}, hashCheck);
	ree_if(footer.Hash != hashCheck, "Failed to check hash. footer.Hash: %u. hashCheck: %u.", footer.Hash, hashCheck);
	::gpk::array_pod<byte_t>							decrypted;
	gpk_necall(::gpk::gpcDecodeWithHash({(uint64_t*)&input[sizeof(::SSessionPayloadHeader)], (uint32_t)header.Size / sizeof(uint64_t)}, client.RSAKeys.PrivateN, client.RSAKeys.Private, true, decrypted), "Failed to decrypt session message.");
	uint32_t											outputOffset										= output.size();
	output.resize(decrypted.size() + outputOffset);
	memcpy(&output[outputOffset], decrypted.begin(), decrypted.size());
	return 0; 
}	// 
