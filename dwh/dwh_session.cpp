#include "dwh_session.h"
#include "gpk_rsa.h"

static constexpr	const uint32_t				SIZE_MAX_CERTIFICATE										= 2048;
static constexpr	const uint32_t				SIZE_MAX_KEY												= 256;
static constexpr	const uint32_t				SIZE_MAX_CLIENT_ID											= 256;

#pragma pack(push, 1)
struct SAuthorityClientIdentifyRequest			{
						byte_t						Certificate			[SIZE_MAX_CERTIFICATE]					= {};
};

struct SAuthorityServerIdentifyResponse			{
						uint64_t					KeysPublic			[SIZE_MAX_KEY]							= {};
						uint64_t					ClientId			[SIZE_MAX_CLIENT_ID]					= {};
};

struct SSessionClientStart						{
						uint64_t					ClientId			[SIZE_MAX_CLIENT_ID]					= {};
};

struct SAuthorityServiceConfirmClientRequest	{
						uint64_t					ClientId			[SIZE_MAX_CLIENT_ID]					= {};
};

struct SAuthorityServerConfirmClientResponse	{
						uint64_t					KeysPrivate			[SIZE_MAX_KEY]							= {};
};

struct SSessionServerAccept						{
						uint64_t					KeysSymmetric		[SIZE_MAX_KEY]							= {};
};

//struct SSessionClientAccepted					{};

#pragma pack(pop)

// Client sends a request to the authority server and software identifier.																
					::gpk::error_t				dwh::authorityClientIdentifyRequest					(::dwh::SSessionClient		& client	, ::gpk::array_pod<byte_t> & output)												{ ;   client			, output; 
	::SAuthorityClientIdentifyRequest					dataToSend											= {};

	// Fill initial request with certificate and client information.
	for(uint32_t iByte = 0; iByte < ::gpk::size(dataToSend.Certificate); ++iByte)	
		dataToSend.Certificate[iByte]					= (ubyte_t)(::gpk::size(dataToSend.Certificate) - 1 - iByte);

	gpk_necall(output.resize(sizeof(::SAuthorityClientIdentifyRequest)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(::SAuthorityClientIdentifyRequest));
	return 0;		 
}

// Authority's server sends a request with the public key for session startup.															
					::gpk::error_t				dwh::authorityServerIdentifyResponse				(::dwh::SSessionAuthority	& authority	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   authority	, input	, output; 
	const ::SAuthorityClientIdentifyRequest				& dataReceived										= *(::SAuthorityClientIdentifyRequest*)input.begin();

	bool												validCertificate									= true;
	// Evaluate certificate and client information and send public keys to client.
	for(uint32_t iByte = 0; iByte < ::gpk::size(dataReceived.Certificate); ++iByte)
		if(dataReceived.Certificate[iByte] != (ubyte_t)(::gpk::size(dataReceived.Certificate) - 1 - iByte))
			validCertificate								= false;

	// Evaluate certificate and client information and send public keys to client.

	::SAuthorityServerIdentifyResponse					dataToSend											= {};
	gpk_necall(output.resize(sizeof(::SAuthorityServerIdentifyResponse)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(::SAuthorityServerIdentifyResponse));
	return 0; 
}	

// Client sends a request to the service's server.																						
					::gpk::error_t				dwh::sessionClientStart								(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   client	, input	, output; 
	::SAuthorityServerIdentifyResponse					& dataReceived										= *(::SAuthorityServerIdentifyResponse*)input.begin();

	// Build service connection request encrypted with the public key received from the authority server.
	dataReceived;

	::SSessionClientStart								dataToSend											= {};
	gpk_necall(output.resize(sizeof(SSessionClientStart)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SSessionClientStart));
	return 0; 
}	

// Service server sends a request to authority server with client information.															
					::gpk::error_t				dwh::authorityServiceConfirmClientRequest			(::dwh::SSessionService		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   service	, input	, output; 
	::SSessionClientStart								dataReceived										= *(::SSessionClientStart*)input.begin();

	// Build request with client information in order to query the authority server for the private key.
	dataReceived;

	::SAuthorityServiceConfirmClientRequest				dataToSend											= {};
	gpk_necall(output.resize(sizeof(SAuthorityServiceConfirmClientRequest)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SAuthorityServiceConfirmClientRequest));
	return 0; 
}	

// Authority server sends a response to service with client information and the private key for decrypting the client password.			
					::gpk::error_t				dwh::authorityServerConfirmClientResponse			(::dwh::SSessionAuthority	& authority	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   authority	, input	, output; 
	::SAuthorityServiceConfirmClientRequest				dataReceived										= *(::SAuthorityServiceConfirmClientRequest*)input.begin();

	// Build response with private key information in order to send to the service and allow it to decrypt the data received from the client.
	dataReceived;

	::SAuthorityServerConfirmClientResponse				dataToSend											= {};
	gpk_necall(output.resize(sizeof(SAuthorityServerConfirmClientResponse)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SAuthorityServerConfirmClientResponse));
	return 0; 
}	

// Service server sends a response to the client containing the symmetric keys for the rest of the communication.						
					::gpk::error_t				dwh::sessionServerAccept							(::dwh::SSessionService		& service	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   service	, input	, output; 
	::SAuthorityServerConfirmClientResponse				dataReceived										= *(::SAuthorityServerConfirmClientResponse*)input.begin();

	// Decrypt client data and build accept/reject response to be sent to the client.
	dataReceived;

	::SSessionServerAccept								dataToSend											= {};
	gpk_necall(output.resize(sizeof(SSessionServerAccept)), "Out of memory?");
	memcpy(output.begin(), &dataToSend, sizeof(SSessionServerAccept));
	return 0; 
}	

// Client processes service response in order to determine if the connection was accepted as legitimate and loads the symmetric keys.	
					::gpk::error_t				dwh::sessionClientAccepted							(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output)				{ ;   client	, input	, output; 
	::SSessionServerAccept								dataReceived										= *(::SSessionServerAccept*)input.begin();

	// Evaluate server response to determine if we can enter into IDLE state or not.
	dataReceived;

	//::SSessionClientAccepted								dataToSend											= {};
	//gpk_necall(output.resize(sizeof(SSessionClientAccepted)), "Out of memory?");
	//memcpy(output.begin(), &dataToSend, sizeof(SSessionClientAccepted));
	return 0; 
}	

					::gpk::error_t				dwh::sessionClientEncrypt							(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output) { client, input, output; return 0; }	// 
					::gpk::error_t				dwh::sessionClientDecrypt							(::dwh::SSessionClient		& client	, ::gpk::view_array<byte_t> & input, ::gpk::array_pod<byte_t> & output) { client, input, output; return 0; }	// 
