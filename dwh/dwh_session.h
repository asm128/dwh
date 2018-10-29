#include "gpk_array.h"

#ifndef DWH_SESSION_H_2987492834
#define DWH_SESSION_H_2987492834

namespace dwh
{
	struct SSessionClient {};
	struct SSessionServer { ::gpk::array_pod<SSessionClient> Clients; };

	::gpk::error_t							sessionClientStart							(::gpk::array_pod<byte_t> & output);
	::gpk::error_t							sessionClientLogin							(::gpk::array_pod<byte_t> & output);
	::gpk::error_t							sessionClientClose							(::gpk::array_pod<byte_t> & output);

	::gpk::error_t							sessionServerClientStart					(::gpk::array_pod<byte_t> & output);
	::gpk::error_t							sessionServerClientLogin					(::gpk::array_pod<byte_t> & output);
	::gpk::error_t							sessionServerClientClose					(::gpk::array_pod<byte_t> & output);
}

#endif // DWH_SESSION_H_2987492834
