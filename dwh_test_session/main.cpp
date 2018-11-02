#include "dwh_session.h"
#include "gpk_sync.h"

int												main									()					{
	::dwh::SSessionAuthority							authority								= {};
	::dwh::SSessionServer								service									= {};
	::gpk::array_pod <byte_t>							output, input							= {}; 

	::gpk::error_t										result									= 0;
	::gpk::sleep(10);
	for(uint32_t i=0; i < 3; ++i) {
		::dwh::SSessionClient								client									= {};
	// ----------- Client reports to authority and receives keys and session information - BEGIN
		gpk_necall(result = ::dwh::authorityClientIdentifyRequest			(client		, output)		, "%s", "Unknown error.");	always_printf("Client    -> authorityClientIdentifyRequest        result: %x.", result);	// 0. Client sends a request to the authority server and software identifier.																
		gpk_necall(result = ::dwh::authorityServerIdentifyResponse			(authority	, output, input), "%s", "Unknown error.");	always_printf("Authority -> authorityServerIdentifyResponse       result: %x.", result);	// 1. Authority's server sends a request with the public key for session startup.															
	// ----------- Client reports to authority and receives keys and session information - END

	// ----------- Client connects to the service - BEGIN
		gpk_necall(result = ::dwh::sessionClientStart						(client		, input, output), "%s", "Unknown error.");	always_printf("Client    -> sessionClientStart                    result: %x.", result);	// 2. Client sends a request to the service's server.																						
	// ----------- Server checks with authority for validity of the client requesting connection - BEGIN
		gpk_necall(result = ::dwh::authorityServiceConfirmClientRequest		(service	, output, input), "%s", "Unknown error.");	always_printf("Server    -> authorityServiceConfirmClientRequest  result: %x.", result);	// 3. Service server sends a request to authority server with client information.															
		gpk_necall(result = ::dwh::authorityServerConfirmClientResponse		(authority	, input, output), "%s", "Unknown error.");	always_printf("Authority -> authorityServerConfirmClientResponse  result: %x.", result);	// 4. Authority server sends a response to service with client information and the private key for decrypting the client password.			
	// ----------- Server checks with authority for validity of the client requesting connection - END
		gpk_necall(result = ::dwh::sessionServerAccept						(service	, output, input), "%s", "Unknown error.");	always_printf("Server    -> sessionServerAccept                   result: %x.", result);	// 5. Service server sends a response to the client containing the symmetric keys for the rest of the communication.						
		gpk_necall(result = ::dwh::sessionClientAccepted					(client		, input, output), "%s", "Unknown error.");	always_printf("Client    -> sessionClientAccepted                 result: %x.", result);	// 6. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
	// ----------- Client connects to the service - END
		always_printf("Client connected. Symmetric key: %llx.", client.KeySymmetric);

	// ----------- 
		{
			input										= "Client encryption test";
			output	.clear(); 
			gpk_necall(::dwh::sessionClientEncrypt(client, input, output), "%s", "Unknown error.");	// 7. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	

			::gpk::array_pod <byte_t>						testServerDecrypt						= {}; 
			gpk_necall(::dwh::sessionServerDecrypt(service, client.IdClient, output, testServerDecrypt), "%s", "Unknown error.");	// 8. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
			error_if(testServerDecrypt.size() != input.size(), "%s", "Client failed to decrypt.");
			error_if(memcmp(testServerDecrypt.begin(), input.begin(), testServerDecrypt.size()), "%s", "Server failed to decrypt.");
			always_printf("Message decrypted by server: %s.", testServerDecrypt.begin());
		}
		{
			input										= "Server encryption test";
			output.clear(); 
			gpk_necall(::dwh::sessionServerEncrypt(service, client.IdClient, input, output), "%s", "Unknown error.");	// 7. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	

			::gpk::array_pod <byte_t>						testClientDecrypt						= {}; 
			gpk_necall(::dwh::sessionClientDecrypt(client, output, testClientDecrypt), "%s", "Unknown error.");	// 8. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
			error_if(testClientDecrypt.size() != input.size(), "%s", "Client failed to decrypt.");
			error_if(memcmp(testClientDecrypt.begin(), input.begin(), testClientDecrypt.size()), "%s", "Client failed to decrypt.");
			always_printf("Message decrypted by client: %s.", testClientDecrypt.begin());
		}
	}
	return 0;
}