#include "dwh_session.h"
#include "gpk_sync.h"
#include "gpk_timer.h"
#include "gpk_aes.h"

int												main									()					{
	::dwh::SSessionAuthority							authority								= {};
	::dwh::SSessionServer								service									= {};
	service.IdServer								= 0;
	::gpk::array_pod <byte_t>							output, input							= {}; 

	::gpk::error_t										result									= 0;
	::gpk::sleep(10);
	double												millisec								= 0;
	for(uint32_t i=0; i < 3; ++i) {
		::dwh::SSessionClient								client									= {};
		client.IdServer									= 0;
		{
			::gpk::STimer															timer;
		// ----------- Client reports to authority and receives keys and session information - BEGIN
			timer.Frame(); gpk_necall(result = ::dwh::authorityClientIdentifyRequest			(client		, output)		, "%s", "Unknown error.");	timer.Frame(); millisec = timer.LastTimeMicroseconds / 1000.0; always_printf("Client    -> authorityClientIdentifyRequest        result: %x. Time: %g milliseconds.", result, millisec);	// 0. Client sends a request to the authority server and software identifier.																
			timer.Frame(); gpk_necall(result = ::dwh::authorityServerIdentifyResponse			(authority	, output, input), "%s", "Unknown error.");	timer.Frame(); millisec = timer.LastTimeMicroseconds / 1000.0; always_printf("Authority -> authorityServerIdentifyResponse       result: %x. Time: %g milliseconds.", result, millisec);	// 1. Authority's server sends a request with the public key for session startup.															
		// ----------- Client reports to authority and receives keys and session information - END

		// ----------- Client connects to the service - BEGIN
			timer.Frame(); gpk_necall(result = ::dwh::sessionClientStart						(client		, input, output), "%s", "Unknown error.");	timer.Frame(); millisec = timer.LastTimeMicroseconds / 1000.0; always_printf("Client    -> sessionClientStart                    result: %x. Time: %g milliseconds.", result, millisec);	// 2. Client sends a request to the service's server.																						
		// ----------- Server checks with authority for validity of the client requesting connection - BEGIN
			timer.Frame(); gpk_necall(result = ::dwh::authorityServiceConfirmClientRequest		(service	, output, input), "%s", "Unknown error.");	timer.Frame(); millisec = timer.LastTimeMicroseconds / 1000.0; always_printf("Server    -> authorityServiceConfirmClientRequest  result: %x. Time: %g milliseconds.", result, millisec);	// 3. Service server sends a request to authority server with client information.															
			timer.Frame(); gpk_necall(result = ::dwh::authorityServerConfirmClientResponse		(authority	, input, output), "%s", "Unknown error.");	timer.Frame(); millisec = timer.LastTimeMicroseconds / 1000.0; always_printf("Authority -> authorityServerConfirmClientResponse  result: %x. Time: %g milliseconds.", result, millisec);	// 4. Authority server sends a response to service with client information and the private key for decrypting the client password.			
		// ----------- Server checks with authority for validity of the client requesting connection - END	
			timer.Frame(); gpk_necall(result = ::dwh::sessionServerAccept						(service	, output, input), "%s", "Unknown error.");	timer.Frame(); millisec = timer.LastTimeMicroseconds / 1000.0; always_printf("Server    -> sessionServerAccept                   result: %x. Time: %g milliseconds.", result, millisec);	// 5. Service server sends a response to the client containing the symmetric keys for the rest of the communication.						
			timer.Frame(); gpk_necall(result = ::dwh::sessionClientAccepted						(client		, input, output), "%s", "Unknown error.");	timer.Frame(); millisec = timer.LastTimeMicroseconds / 1000.0; always_printf("Client    -> sessionClientAccepted                 result: %x. Time: %g milliseconds.", result, millisec);	// 6. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
		// ----------- Client connects to the service - END
			always_printf("Client connected. Symmetric key: %llx.", client.KeySymmetric);
		}
	// ----------- 
		{
			input										= "Client encryption test";
			output	.clear(); 
			::gpk::STimer									timer;
			gpk_necall(::dwh::sessionClientEncrypt(client, input, output), "%s", "Unknown error.");	// 7. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
			timer.Frame();
			millisec = timer.LastTimeMicroseconds / 1000.0; 
			always_printf("Message encrypted by client in %g milliseconds: %s.", millisec, input.begin());

			::gpk::array_pod <byte_t>						testServerDecrypt						= {}; 
			gpk_necall(::dwh::sessionServerDecrypt(service, client.IdClient, output, testServerDecrypt), "%s", "Unknown error.");	// 8. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
			error_if(testServerDecrypt.size() != input.size(), "%s", "Client failed to decrypt.");
			timer.Frame();
			millisec = timer.LastTimeMicroseconds / 1000.0; 
			error_if(memcmp(testServerDecrypt.begin(), input.begin(), testServerDecrypt.size()), "%s", "Server failed to decrypt.");
			always_printf("Message decrypted by server in %g milliseconds: %s.", millisec, testServerDecrypt.begin());
		}
		{
			input										= "Server encryption test";
			output.clear(); 
			::gpk::STimer									timer;
			gpk_necall(::dwh::sessionServerEncrypt(service, client.IdClient, input, output), "%s", "Unknown error.");	// 7. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
			timer.Frame();
			millisec = timer.LastTimeMicroseconds / 1000.0; 
			always_printf("Message encrypted by server in %g milliseconds: %s.", millisec, input.begin());

			::gpk::array_pod <byte_t>						testClientDecrypt						= {}; 
			gpk_necall(::dwh::sessionClientDecrypt(client, output, testClientDecrypt), "%s", "Unknown error.");	// 8. Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
			error_if(testClientDecrypt.size() != input.size(), "%s", "Client failed to decrypt.");
			timer.Frame();
			millisec = timer.LastTimeMicroseconds / 1000.0; 
			error_if(memcmp(testClientDecrypt.begin(), input.begin(), testClientDecrypt.size()), "%s", "Client failed to decrypt.");
			always_printf("Message decrypted by client in %g milliseconds: %s.", millisec, testClientDecrypt.begin());
		}
		::gpk::array_pod<byte_t>						aesEncrypted;
		::gpk::array_pod<byte_t>						aesDecrypted;
		::gpk::aesEncode("Message", {(ubyte_t*)&client.KeySymmetric[1], 32}, ::gpk::AES_LEVEL_256, aesEncrypted);
		::gpk::aesDecode(aesEncrypted, {(ubyte_t*)&client.KeySymmetric[1], 32}, ::gpk::AES_LEVEL_256, aesDecrypted);
		always_printf("AES decrypted: %s", aesDecrypted.begin());
	}
	return 0;
}
