#include "gpk_array.h"

#ifndef DWH_CLIENT_SESSION_H_20181013
#define DWH_CLIENT_SESSION_H_20181013

namespace dwh
{
	static constexpr	const uint32_t								MAX_CLIENT_TOKEN_SIZE						= 256;

	struct SDWHSessionKeys {
							byte_t										Key0	[128]								= {};
							byte_t										Key1	[128]								= {};
	};

	struct SDWHSession {
							byte_t										Token	[MAX_CLIENT_TOKEN_SIZE]				= {};
	};

	struct SDWHPayloadHeader {
							uint64_t									Size										= 0;
							uint64_t									Time										= 0;
							uint64_t									Hash										= 0;
	};

	struct SDWHPayloadFooter {
							uint64_t									Hash										= 0;
	};

	struct SDWHPayload {
							uint64_t									Time										= 0;
							::gpk::array_pod<byte_t>					Data;
	};

	struct SDWHConnectionQueue {
							::gpk::array_obj<::dwh::SDWHPayload>		Receive;
	};

						::gpk::error_t								payloadEncrypt								(const SDWHSessionKeys & session, const  ::dwh::SDWHPayload& input, ::gpk::array_pod<byte_t>& encrypted);
						::gpk::error_t								payloadDecrypt								(const SDWHSessionKeys & session, const  ::gpk::view_array<byte_t>& input, ::dwh::SDWHPayload& decrypted);
} // namespace

#endif // DWH_CLIENT_SESSION_H_20181013
