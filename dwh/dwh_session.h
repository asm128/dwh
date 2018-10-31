#include "gpk_array.h"
#include "gpk_noise.h"

#ifndef DWH_SESSION_H_2987492834
#define DWH_SESSION_H_2987492834

namespace dwh
{
#pragma pack(push, 1)
// ----------- Client reports to authority and receives keys and session information - BEGIN
// Client		-> Authority			// Client sends a request to the authority server and software identifier.																// authorityClientIdentifyRequest		()
// Authority	-> Client				// Authority's server sends a request with the public key for session startup.															// authorityServerIdentifyResponse		()
// ----------- Client reports to authority and receives keys and session information - END

// ----------- Client connects to the service - BEGIN
// Client		-> Service				// Client sends a request to the service's server.																						// sessionClientStart					()

// ----------- Server checks with authority for validity of the client requesting connection - BEGIN
// Service		-> Authority			// Service server sends a request to authority server with client information.															// authorityServiceConfirmClientRequest	()
// Authority	-> Service				// Authority server sends a response to service with client information and the private key for decrypting the client password.				// authorityServerConfirmClientResponse	()
// ----------- Server checks with authority for validity of the client requesting connection - END

// Service		-> Client				// Service server sends a response to the client containing the symmetric keys for the rest of the communication.						// sessionServerAccept					()
// Client		-> IDLE					// Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	// sessionClientAccepted				()
// ----------- Client connects to the service - END
	enum SESSION_STAGE : uint8_t
		{ SESSION_STAGE_CLIENT_CLOSED					= 0	// sessionClose							()
		, SESSION_STAGE_CLIENT_IDENTIFY						// authorityClientIdentifyRequest		() // Client	-> Authority			// Processed by authority
		, SESSION_STAGE_AUTHORITY_IDENTIFY					// authorityServerIdentifyResponse		() // Authority	-> Client				// Processed by client
		, SESSION_STAGE_CLIENT_REQUEST_SERVICE_START		// sessionClientStart					() // Client	-> Service				// Processed by service
		, SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST		// authorityServiceConfirmClientRequest	() // Service	-> Authority			// Processed by authority
		, SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT			// authorityServerConfirmClientResponse	() // Authority	-> Service				// Processed by service
		, SESSION_STAGE_SERVER_ACCEPT_CLIENT				// sessionServerAccept					() // Service		-> Client			// Processed by client
		, SESSION_STAGE_CLIENT_IDLE							// sessionClientAccepted				() // Client		-> IDLE				// Processed by client/service
		};

	struct SKeyPair {
		uint64_t								Public;
		uint64_t								Private;
		uint64_t								PublicN;
		uint64_t								PrivateN;
	};

	struct SSessionClient		{ 
		SESSION_STAGE							Stage										= SESSION_STAGE_CLIENT_CLOSED; 
		uint64_t								IdClient									= (uint64_t)-1LL;
		uint64_t								IdServer									= (uint64_t)-1LL;
		SKeyPair								RSAKeys										= {};
		uint64_t								KeySymmetric								= 0;
	};

	struct SSessionClientRecord	{ 
		SESSION_STAGE							Stage										= SESSION_STAGE_CLIENT_CLOSED; 
		uint64_t								IdClient									= (uint64_t)-1LL;
		SKeyPair								RSAKeysClient								= {};
		SKeyPair								RSAKeysServer								= {};
	};

#pragma pack(pop)

	struct SSessionServer		{ ::gpk::array_pod<SSessionClientRecord	> Clients; };

	struct SSessionAuthority	{ ::gpk::array_obj<SSessionServer		> Servers; };

						::gpk::error_t		authorityClientIdentifyRequest				(::dwh::SSessionClient		& client	, ::gpk::array_pod <byte_t> & output);										// 0. Client sends a request to the authority server and software identifier.																
						::gpk::error_t		authorityServerIdentifyResponse				(::dwh::SSessionAuthority	& authority	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 1. Authority's server sends a request with the public key for session startup.															
						::gpk::error_t		sessionClientStart							(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 2. Client sends a request to the service's server.																						
						::gpk::error_t		authorityServiceConfirmClientRequest		(::dwh::SSessionServer		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 3. Service server sends a request to authority server with client information.															
						::gpk::error_t		authorityServerConfirmClientResponse		(::dwh::SSessionAuthority	& authority	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 4. Authority server sends a response to service with client information and the private key for decrypting the client password.			
						::gpk::error_t		sessionServerAccept							(::dwh::SSessionServer		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 5. Service server sends a response to the client containing the symmetric keys for the rest of the communication.						
						::gpk::error_t		sessionClientAccepted						(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 6. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	

						::gpk::error_t		sessionClientEncrypt						(::dwh::SSessionClient		& client	, const ::gpk::view_array<const byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 7. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
						::gpk::error_t		sessionClientDecrypt						(::dwh::SSessionClient		& client	, const ::gpk::view_array<const byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 8. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
						::gpk::error_t		sessionServerEncrypt						(::dwh::SSessionServer		& client	, uint64_t idClient, const ::gpk::view_array<const byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 7. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
						::gpk::error_t		sessionServerDecrypt						(::dwh::SSessionServer		& client	, uint64_t idClient, const ::gpk::view_array<const byte_t> & input, ::gpk::array_pod<byte_t> & output);	// 8. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
						::gpk::error_t		sessionHash									(::dwh::SSessionClient		& client	, const ::gpk::view_array<const byte_t> & input, uint64_t & output);

	static constexpr	const uint64_t		NOISE_SEED									= ::gpk::NOISE_SEED;
}


#endif // DWH_SESSION_H_2987492834
