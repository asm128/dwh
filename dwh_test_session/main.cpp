#include "dwh_session.h"

int												main									()					{
	::dwh::SSessionAuthority							authority								= {};
	::dwh::SSessionClient								client									= {};
	::dwh::SSessionServer								service									= {};
	::gpk::array_pod <byte_t>							output, input							= {}; 

// ----------- Client reports to authority and receives keys and session information - BEGIN
	gpk_necall(::dwh::authorityClientIdentifyRequest			(client		, output)		, "%s", "Unknown error.");	// 0. Client sends a request to the authority server and software identifier.																
	gpk_necall(::dwh::authorityServerIdentifyResponse			(authority	, output, input), "%s", "Unknown error.");	// 1. Authority's server sends a request with the public key for session startup.															
// ----------- Client reports to authority and receives keys and session information - END

// ----------- Client connects to the service - BEGIN
	gpk_necall(::dwh::sessionClientStart						(client		, input, output), "%s", "Unknown error.");	// 2. Client sends a request to the service's server.																						

// ----------- Server checks with authority for validity of the client requesting connection - BEGIN
	gpk_necall(::dwh::authorityServiceConfirmClientRequest		(service	, output, input), "%s", "Unknown error.");	// 3. Service server sends a request to authority server with client information.															
	gpk_necall(::dwh::authorityServerConfirmClientResponse		(authority	, input, output), "%s", "Unknown error.");	// 4. Authority server sends a response to service with client information and the private key for decrypting the client password.			
// ----------- Server checks with authority for validity of the client requesting connection - END

	gpk_necall(::dwh::sessionServerAccept						(service	, output, input), "%s", "Unknown error.");	// 5. Service server sends a response to the client containing the symmetric keys for the rest of the communication.						
	gpk_necall(::dwh::sessionClientAccepted						(client		, input, output), "%s", "Unknown error.");	// 6. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	

	info_printf("Client connected. Symmetric key: %llu.", client.KeySymmetric);
// ----------- Client connects to the service - END

// ----------- 
	{
		input										= "Client encryption test";
		output	.clear(); 
		gpk_necall(::dwh::sessionClientEncrypt(client, input, output), "%s", "Unknown error.");	// 7. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	

		::gpk::array_pod <byte_t>						testServerDecrypt						= {}; 
		gpk_necall(::dwh::sessionServerDecrypt(service, client.IdClient, output, testServerDecrypt), "%s", "Unknown error.");	// 8. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
		error_if(testServerDecrypt.size() != input.size(), "%s", "Client failed to decrypt.");
		error_if(memcmp(testServerDecrypt.begin(), input.begin(), testServerDecrypt.size()), "%s", "Server failed to decrypt.");
		info_printf("Message decrypted by server: %s.", testServerDecrypt.begin());
	}
	{
		input										= "Server encryption test";
		output.clear(); 
		gpk_necall(::dwh::sessionServerEncrypt(service, client.IdClient, input, output), "%s", "Unknown error.");	// 7. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	

		::gpk::array_pod <byte_t>						testClientDecrypt						= {}; 
		gpk_necall(::dwh::sessionClientDecrypt(client, output, testClientDecrypt), "%s", "Unknown error.");	// 8. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
		error_if(testClientDecrypt.size() != input.size(), "%s", "Client failed to decrypt.");
		error_if(memcmp(testClientDecrypt.begin(), input.begin(), testClientDecrypt.size()), "%s", "Client failed to decrypt.");
		info_printf("Message decrypted by client: %s.", testClientDecrypt.begin());
	}
	return 0;
}