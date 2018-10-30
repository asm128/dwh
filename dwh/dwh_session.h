#include "gpk_array.h"

#ifndef DWH_SESSION_H_2987492834
#define DWH_SESSION_H_2987492834

namespace dwh
{
	enum DWH_SESSION_STAGE 
		{ DWH_SESSION_STAGE_CLIENT_IDENTIFY						= 0	// authorityClientIdentifyRequest		()
		, DWH_SESSION_STAGE_AUTHORITY_IDENTIFY						// authorityServerIdentifyResponse		()
		, DWH_SESSION_STAGE_CLIENT_REQUEST_SERVICE_START			// sessionClientStart					()
		, DWH_SESSION_STAGE_SERVICE_EVALUATE_CLIENT_REQUEST			// authorityServiceConfirmClientRequest	()
		, DWH_SESSION_STAGE_AUTHORITY_CONFIRM_CLIENT				// authorityServerConfirmClientResponse	()
		, DWH_SESSION_STAGE_SERVER_ACCEPT_CLIENT					// sessionServerAccept					()
		, DWH_SESSION_STAGE_CLIENT_IDLE								// sessionClientAccepted				()
		};

	struct SSessionClient		{};
	struct SSessionClientRecord	{};
	struct SSessionService		{ ::gpk::array_pod<SSessionClientRecord> Clients; };
	struct SSessionAuthority	{ ::gpk::array_pod<SSessionClientRecord> Clients; };

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

	::gpk::error_t							authorityClientIdentifyRequest				(::dwh::SSessionClient		& client	, ::gpk::array_pod<byte_t> & output);										// Client sends a request to the authority server and software identifier.																
	::gpk::error_t							authorityServerIdentifyResponse				(::dwh::SSessionAuthority	& authority	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// Authority's server sends a request with the public key for session startup.															
	::gpk::error_t							sessionClientStart							(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// Client sends a request to the service's server.																						
	::gpk::error_t							authorityServiceConfirmClientRequest		(::dwh::SSessionService		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// Service server sends a request to authority server with client information.															
	::gpk::error_t							authorityServerConfirmClientResponse		(::dwh::SSessionAuthority	& authority	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// Authority server sends a response to service with client information and the private key for decrypting the client password.			
	::gpk::error_t							sessionServerAccept							(::dwh::SSessionService		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// Service server sends a response to the client containing the symmetric keys for the rest of the communication.						
	::gpk::error_t							sessionClientAccepted						(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	

	::gpk::error_t							sessionClientEncrypt						(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
	::gpk::error_t							sessionClientDecrypt						(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output);	// Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
}


#endif // DWH_SESSION_H_2987492834
