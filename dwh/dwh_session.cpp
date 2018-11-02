#include "dwh_session.h"
#include "gpk_rsa.h"
#include "gpk_chrono.h"
#include "gpk_noise.h"
#include "gpk_encoding.h"

static constexpr	const uint32_t				SIZE_MAX_CERTIFICATE												= 1024;
static constexpr	const uint32_t				SIZE_MAX_KEY														= 1024;
static constexpr	const uint32_t				SIZE_MAX_CLIENT_ID													= 1024;
	
#pragma pack(push, 1)	
	
struct SAuthorityClientIdentifyRequest			{	
						::dwh::SSessionCommand		Command																= {::dwh::SESSION_STAGE_CLIENT_CLOSED};
						uint64_t					IdServer															= (uint64_t)-1LL;
						uint64_t					Certificate			[SIZE_MAX_CERTIFICATE - 1 + SIZE_MAX_KEY]		= {};
};	
	
struct SAuthorityServerIdentifyResponse			{	
						::dwh::SSessionCommand		Command																= {::dwh::SESSION_STAGE_CLIENT_CLOSED};
						::dwh::SKeyPair				Keys																= {};
						uint64_t					IdClient			[SIZE_MAX_CLIENT_ID - 4 + SIZE_MAX_KEY]			= {};
};	
	
struct SSessionClientStart						{	
						::dwh::SSessionCommand		Command																= {::dwh::SESSION_STAGE_CLIENT_CLOSED};
						uint64_t					IdClient			[SIZE_MAX_CLIENT_ID]							= {};
						uint64_t					KeySize																= 0;
						uint64_t					KeysSymmetric		[SIZE_MAX_KEY - 1]								= {};
};	
	
struct SAuthorityServiceConfirmClientRequest	{	
						::dwh::SSessionCommand		Command																= {::dwh::SESSION_STAGE_CLIENT_CLOSED};
						uint64_t					IdServer															= {};
						uint64_t					IdClient			[SIZE_MAX_CLIENT_ID - 1 + 1024]					= {};
};	
	
struct SAuthorityServerConfirmClientResponse	{	
						::dwh::SSessionCommand		Command																= {::dwh::SESSION_STAGE_CLIENT_CLOSED};
						::dwh::SKeyPair				KeysServer															= {};
						::dwh::SKeyPair				KeysClient															= {};
						uint64_t					IdClient			[SIZE_MAX_CLIENT_ID - 8 + 1024]					= {};
};	
	
struct SSessionServerAccept	{	
						::dwh::SSessionCommand		Command																= {::dwh::SESSION_STAGE_CLIENT_CLOSED};
						uint64_t					KeySize																= 0;
						::dwh::SKeyPair				KeysClient			[511]											= {};
						char						Padding				[23];
};

static constexpr const size_t z = sizeof(::dwh::SKeyPair);
static constexpr const size_t a = sizeof(SAuthorityClientIdentifyRequest);
static constexpr const size_t b = sizeof(SAuthorityServerIdentifyResponse);
static constexpr const size_t c = sizeof(SSessionClientStart);
static constexpr const size_t d = sizeof(SAuthorityServiceConfirmClientRequest);
static constexpr const size_t e = sizeof(SAuthorityServerConfirmClientResponse);
static constexpr const size_t f = sizeof(SSessionServerAccept);
//struct SSessionClientAccepted					{};
#pragma pack(pop)

// Client sends a request to the authority server and software identifier.																
					::gpk::error_t				dwh::authorityClientIdentifyRequest					(::dwh::SSessionClient		& client	, ::gpk::array_pod<byte_t> & output)												{ ;   client			, output; 
	client.Stage									= ::dwh::SESSION_STAGE_CLIENT_IDENTIFY;
	client.IdServer									= 0;
	client.IdClient									= (uint64_t)-1LL;
	client.RSAKeys									= {};

	::SAuthorityClientIdentifyRequest					dataToSend											= {};
	dataToSend.Command.Command						= ::dwh::SESSION_STAGE_CLIENT_IDENTIFY;
	// Fill initial request with certificate and client information.
	for(uint32_t iByte = 0; iByte < ::gpk::size(dataToSend.Certificate); ++iByte)	
		dataToSend.Certificate[iByte]					= (ubyte_t)(::gpk::size(dataToSend.Certificate) - 1 - iByte);
	dataToSend.IdServer								= client.IdServer;
	gpk_necall(output.resize(sizeof(::SAuthorityClientIdentifyRequest)), "%s", "Out of memory?");
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
	uint64_t											idClient											= (uint64_t)-1LL;
	::dwh::SKeyPair										keysClient											= {};
	int32_t												indexClient											= -1;
	if(false == validServerId) 
	{}
	else {
		// Evaluate certificate and client information.
		for(uint32_t iByte = 0; iByte < ::gpk::size(dataReceived.Certificate); ++iByte)
			if(dataReceived.Certificate[iByte] != (ubyte_t)(::gpk::size(dataReceived.Certificate) - 1 - iByte)) 
				validCertificate								= false;
	
		idClient										= validCertificate ? ::gpk::timeCurrentInUs() : (uint64_t)0;
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

			newClient.RSAKeysServer.Private					= clienttoserverprivate;
			newClient.RSAKeysServer.PrivateN				= clienttoservern;
			newClient.RSAKeysServer.Public					= servertoclientpublic;
			newClient.RSAKeysServer.PublicN					= servertoclientn;		

			gpk_necall(indexClient = authority.Servers[(uint32_t)dataReceived.IdServer].Clients.push_back(newClient), "%s", "Out of memory?");

			keysClient										= newClient.RSAKeysClient;
		}
	}

	// Send keys to client.
	::SAuthorityServerIdentifyResponse					dataToSend											= {};
	dataToSend.Command.Command						= ::dwh::SESSION_STAGE_AUTHORITY_IDENTIFY;
	dataToSend.Keys									= keysClient;
	dataToSend.Keys.Private							= 0;
	dataToSend.Keys.PrivateN						= 0;
	if(false == validCertificate) {
		//memset(dataToSend.IdClient, 0xFF, ::gpk::size(dataToSend.IdClient) * sizeof(dataToSend.IdClient[0]));
		dataToSend.IdClient[0]							= idClient;
		dataToSend.Keys.Public							= (uint64_t)-1LL;
	}
	else if(false == validServerId) {
		//memset(dataToSend.IdClient, 0xFF, ::gpk::size(dataToSend.IdClient) * sizeof(dataToSend.IdClient[0]));
		dataToSend.IdClient[0]							= idClient;
		dataToSend.Keys.Public							= (uint64_t)-2LL;
	}
	else {
		dataToSend.IdClient[0]							= idClient;
	}
	gpk_necall(output.resize(sizeof(::SAuthorityServerIdentifyResponse)), "%s", "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(::SAuthorityServerIdentifyResponse));
	return indexClient; 
}	

// Client sends a request to the service's server.																						
					::gpk::error_t				dwh::sessionClientStart								(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   client	, input	, output; 
	ree_if(client.Stage != SESSION_STAGE_CLIENT_IDENTIFY, "Invalid client stage: %x.", client.Stage);
	client.Stage									= ::dwh::SESSION_STAGE_CLIENT_REQUEST_SERVICE_START;
	::SAuthorityServerIdentifyResponse					& dataReceived										= *(::SAuthorityServerIdentifyResponse*)input.begin();

	ree_if(errored(client.IdClient = dataReceived.IdClient[0]), "%s", "Client not authorized.");
	client.RSAKeys									= dataReceived.Keys;

	// Build service connection request encrypted with the public key received from the authority server.

	uint64_t											keySymmetric										= ::gpk::noise1DBase(::gpk::timeCurrentInUs() + ::gpk::noise1DBase(::gpk::timeCurrentInUs()), ::gpk::timeCurrentInUs() * 32693) + ::gpk::timeCurrentInUs();
	::SSessionClientStart								dataToSend											= {};
	dataToSend.Command.Command						= ::dwh::SESSION_STAGE_CLIENT_REQUEST_SERVICE_START;
	dataToSend.IdClient[0]							= client.IdClient;
	dataToSend.KeysSymmetric[0]						= client.KeySymmetric[0]								= keySymmetric;
	dataToSend.KeysSymmetric[1]						= client.KeySymmetric[1]								= ::gpk::noise1DBase(::gpk::timeCurrentInUs() + ::gpk::noise1DBase(::gpk::timeCurrentInUs()), ::gpk::timeCurrentInUs() * 32719) + ::gpk::timeCurrentInUs();
	dataToSend.KeysSymmetric[2]						= client.KeySymmetric[2]								= ::gpk::noise1DBase(::gpk::timeCurrentInUs() + ::gpk::noise1DBase(::gpk::timeCurrentInUs()), ::gpk::timeCurrentInUs() * 32707) + ::gpk::timeCurrentInUs();
	dataToSend.KeysSymmetric[3]						= client.KeySymmetric[3]								= ::gpk::noise1DBase(::gpk::timeCurrentInUs() + ::gpk::noise1DBase(::gpk::timeCurrentInUs()), ::gpk::timeCurrentInUs() * 32713) + ::gpk::timeCurrentInUs();
	dataToSend.KeysSymmetric[4]						= client.KeySymmetric[4]								= ::gpk::noise1DBase(::gpk::timeCurrentInUs() + ::gpk::noise1DBase(::gpk::timeCurrentInUs()), ::gpk::timeCurrentInUs() * 32717) + ::gpk::timeCurrentInUs();
	for(uint32_t iKey = 0, countKeys = 5/*::gpk::size(client.KeySymmetric)*/; iKey < countKeys; ++iKey)
		info_printf("Key to send: %llx.", client.KeySymmetric[iKey]);
	::gpk::array_pod<byte_t>							encrypted;
	::dwh::sessionClientEncrypt(client, {(const byte_t*)dataToSend.KeysSymmetric, sizeof(uint64_t) * 5}, encrypted);	// send 5 uint64_t one for Ardell and other 4 for AES (32 bytes)
	dataToSend.KeySize								= encrypted.size();
	info_printf("encrypted key size: %llu.", dataToSend.KeySize);
	memcpy_s(dataToSend.KeysSymmetric, ::gpk::size(dataToSend.KeysSymmetric) * sizeof(dataToSend.KeysSymmetric[0]), encrypted.begin(), dataToSend.KeySize);
	gpk_necall(output.resize(sizeof(SSessionClientStart)), "%s", "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SSessionClientStart));
	return 0; 
}

// Service server sends a request to authority server with client information.															
					::gpk::error_t				dwh::authorityServiceConfirmClientRequest			(::dwh::SSessionServer		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   service	, input	, output; 
	::SSessionClientStart								dataReceived										= *(::SSessionClientStart*)input.begin();

	uint32_t											invalidClient										= (uint32_t)-1;
	for(uint32_t iClient = 0, countClients = service.Clients.size(); iClient < countClients; ++iClient)
		if(service.Clients[iClient].IdClient == dataReceived.IdClient[0]) {
			invalidClient									= iClient;
			error_printf("Client already exists: %llu.", service.Clients[iClient].IdClient);
			memset(dataReceived.IdClient, 0xFF, ::gpk::size(dataReceived.IdClient) * sizeof(dataReceived.IdClient[0]));
		}

	int32_t												validClient											= -1;
	error_if(-1 != invalidClient, "Client id already exists at index: %u. Client id: %llu.", invalidClient, dataReceived.IdClient[0])
	else {
		::dwh::SSessionClientRecord							newClient;
		newClient.Stage									= ::dwh::SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST;
		newClient.IdClient								= dataReceived.IdClient[0];
		newClient.KeySize								= dataReceived.KeySize;
		memcpy_s(newClient.KeySymmetric, ::gpk::size(newClient.KeySymmetric) * sizeof(newClient.KeySymmetric[0]), dataReceived.KeysSymmetric, dataReceived.KeySize);
		validClient										= service.Clients.push_back(newClient);
	}

	// Build request with client information in order to query the authority server for the private key.
	::SAuthorityServiceConfirmClientRequest				dataToSend											= {};
	dataToSend.Command.Command						= ::dwh::SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST;
	dataToSend.IdClient[0]							= dataReceived.IdClient[0];
	dataToSend.IdServer								= 0;
	gpk_necall(output.resize(sizeof(SAuthorityServiceConfirmClientRequest)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SAuthorityServiceConfirmClientRequest));
	return validClient; 
}	

// Authority server sends a response to service with client information and the private key for decrypting the client password.			
					::gpk::error_t				dwh::authorityServerConfirmClientResponse			(::dwh::SSessionAuthority	& authority	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   authority	, input	, output; 
	::SAuthorityServiceConfirmClientRequest				dataReceived										= *(::SAuthorityServiceConfirmClientRequest*)input.begin();

	// Build response with private key information in order to send to the service and allow it to decrypt the data received from the client.
	int32_t												validClient											= -1;
	::dwh::SKeyPair										keysServer											= {};
	::dwh::SKeyPair										keysClient											= {};
	{
		::dwh::SSessionServer								server												= authority.Servers[(uint32_t)dataReceived.IdServer];
		for(uint32_t iClient = 0, countClients = server.Clients.size(); iClient < countClients; ++iClient )
			if(server.Clients[iClient].IdClient == dataReceived.IdClient[0]) {
				keysServer										= server.Clients[iClient].RSAKeysServer;
				keysClient										= server.Clients[iClient].RSAKeysClient;
				validClient										= iClient;
				server.Clients[iClient].Stage					= SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT;
			}
	}

	::SAuthorityServerConfirmClientResponse				dataToSend											= {};
	dataToSend.Command.Command						= ::dwh::SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT;
	dataToSend.KeysServer							= keysServer;				
	dataToSend.KeysClient							= keysClient;				
	dataToSend.KeysClient.Public					= 0;
	dataToSend.KeysClient.PublicN					= 0;
	dataToSend.IdClient[0]							= dataReceived.IdClient[0];
	gpk_necall(output.resize(sizeof(SAuthorityServerConfirmClientResponse)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SAuthorityServerConfirmClientResponse));
	return validClient; 
}	

// Service server sends a response to the client containing the symmetric keys for the rest of the communication.						
					::gpk::error_t				dwh::sessionServerAccept							(::dwh::SSessionServer		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   service	, input	, output; 
	::SAuthorityServerConfirmClientResponse				dataReceived										= *(::SAuthorityServerConfirmClientResponse*)input.begin();

	// Decrypt client data and build accept/reject response to be sent to the client.
	uint32_t											indexClient											= (uint32_t)-1LL;
	uint64_t											keySymmetric										= (uint32_t)-1LL;
	::dwh::SKeyPair										keysClient											= {(uint64_t)-1LL, (uint64_t)-1LL, (uint64_t)-1LL, (uint64_t)-1LL};
	for(uint32_t iClient = 0, countClients = service.Clients.size(); iClient < countClients; ++iClient)
		if(service.Clients[iClient].IdClient == dataReceived.IdClient[0]) {
			SSessionClientRecord							& currentClient										= service.Clients[iClient];
			//keySymmetric								= ::gpk::noise1DBase(::gpk::timeCurrentInUs() + ::gpk::noise1DBase(::gpk::timeCurrentInUs()), ::gpk::timeCurrentInUs() * 32749);
			currentClient.Stage							= ::dwh::SESSION_STAGE_SERVER_ACCEPT_CLIENT;
			currentClient.RSAKeysServer					= dataReceived.KeysServer;
			currentClient.RSAKeysClient					= keysClient											= dataReceived.KeysClient;
			::gpk::array_pod<byte_t>						decrypted;
			::dwh::sessionServerDecrypt(service, currentClient.IdClient, {(const byte_t*)currentClient.KeySymmetric, (uint32_t)currentClient.KeySize}, decrypted);
			info_printf("Encrypted key size received: %llu.", currentClient.KeySize);
			memcpy_s(currentClient.KeySymmetric, ::gpk::size(currentClient.KeySymmetric) * sizeof(currentClient.KeySymmetric[0]), decrypted.begin(), decrypted.size());
			for(uint32_t iKey = 0, countKeys = 5/*::gpk::size(currentClient.KeySymmetric)*/; iKey < countKeys; ++iKey)
				info_printf("Key received: %llx.", currentClient.KeySymmetric[iKey]);
			keySymmetric								= currentClient.KeySymmetric[0];
			info_printf("Decrypted key received: %llx.", keySymmetric);
			indexClient									= iClient;
			break;
		}

	::SSessionServerAccept								dataToSend											= {};
	dataToSend.Command.Command						= ::dwh::SESSION_STAGE_SERVER_ACCEPT_CLIENT;
	if(-1 != (int32_t)indexClient) {
		dataToSend.KeysClient[0]						= keysClient;
		info_printf("Sending key: %llx.", dataToSend.KeysClient[0].Private);
	}
	else {
		memset(dataToSend.KeysClient, 0xFF, ::gpk::size(dataToSend.KeysClient) * sizeof(dataToSend.KeysClient[0]));
		dataToSend.KeysClient[0].Private				= keySymmetric;
		dataToSend.KeysClient[0].PrivateN				= keySymmetric;
		dataToSend.KeysClient[0].Public					= keySymmetric;
		dataToSend.KeysClient[0].PublicN				= keySymmetric;
	}
	::gpk::array_pod<byte_t>							encrypted;
	::gpk::ardellEncode({(const byte_t*)dataToSend.KeysClient, sizeof(::dwh::SKeyPair)}, (int)keySymmetric, true, encrypted);

	dataToSend.KeySize								= encrypted.size();
	memcpy_s(dataToSend.KeysClient, ::gpk::size(dataToSend.KeysClient) * sizeof(dataToSend.KeysClient[0]), encrypted.begin(), dataToSend.KeySize);
	info_printf("Encrypted key: %llx.", dataToSend.KeysClient[0].Private);

	gpk_necall(output.resize(sizeof(SSessionServerAccept)), "%s", "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SSessionServerAccept));
	return indexClient; 
}	

// Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
					::gpk::error_t				dwh::sessionClientAccepted							(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   client	, input	, output; 
	::SSessionServerAccept								dataReceived										= *(::SSessionServerAccept*)input.begin();
	::gpk::array_pod<byte_t>							decrypted;
	::gpk::ardellDecode({(const byte_t*)dataReceived.KeysClient, (uint32_t)dataReceived.KeySize}, (int)client.KeySymmetric[0], true, decrypted);
	memcpy_s(dataReceived.KeysClient, ::gpk::size(dataReceived.KeysClient) * sizeof(dataReceived.KeysClient[0]), decrypted.begin(), decrypted.size());
	client.RSAKeys.Private							= dataReceived.KeysClient[0].Private;
	client.RSAKeys.PrivateN							= dataReceived.KeysClient[0].PrivateN;

	//// Evaluate server response to determine if we can enter into IDLE state or not.
	ree_if(-1LL == (int64_t)(dataReceived.KeysClient[0].Private),  "%s", "Service rejected communication.");
	client.Stage									= ::dwh::SESSION_STAGE_CLIENT_IDLE;
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

static				::gpk::error_t				getClientIndex										(::gpk::array_pod<::dwh::SSessionClientRecord> clients, uint64_t idClient) {
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
	ree_if(footer.Hash != hashCheck, "Failed to check hash. footer.Hash: %llx. hashCheck: %llx.", footer.Hash, hashCheck);
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
	ree_if(footer.Hash != hashCheck, "Failed to check hash. footer.Hash: %llx. hashCheck: %llx.", footer.Hash, hashCheck);
	::gpk::array_pod<byte_t>							decrypted;
	gpk_necall(::gpk::gpcDecodeWithHash({(uint64_t*)&input[sizeof(::SSessionPayloadHeader)], (uint32_t)header.Size / sizeof(uint64_t)}, client.RSAKeys.PrivateN, client.RSAKeys.Private, true, decrypted), "Failed to decrypt session message.");
	uint32_t											outputOffset										= output.size();
	output.resize(decrypted.size() + outputOffset);
	memcpy(&output[outputOffset], decrypted.begin(), decrypted.size());
	return 0; 
}	// 
