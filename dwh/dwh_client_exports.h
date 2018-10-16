#include "gpk_module.h"
#include "gpk_view_array.h"


#ifndef DWH_CLIENT_EXPORTS_H_20181013
#define DWH_CLIENT_EXPORTS_H_20181013

GPK_DECLARE_MODULE_FUNCTION(dwhClientCreate		, );
GPK_DECLARE_MODULE_FUNCTION(dwhClientDestroy	, int32_t clientId)																																			;
GPK_DECLARE_MODULE_FUNCTION(dwhClientDisconnect	, int32_t clientId)																																			;
GPK_DECLARE_MODULE_FUNCTION(dwhClientConnect	, int32_t clientId, int32_t ip0, int32_t ip1, int32_t ip2, int32_t ip3, int32_t port, uint32_t * tokenSize, byte_t * tokenBytes)							;
GPK_DECLARE_MODULE_FUNCTION(dwhClientElevate	, int32_t clientId, uint32_t userSize, const byte_t * userBytes, uint32_t passSize, const byte_t * passBytes, uint32_t * tokenSize, byte_t * tokenBytes)	;
GPK_DECLARE_MODULE_FUNCTION(dwhClientResume		, int32_t clientId, int32_t ip0, int32_t ip1, int32_t ip2, int32_t ip3, int32_t port, uint32_t tokenSize, const byte_t * tokenBytes)						;
GPK_DECLARE_MODULE_FUNCTION(dwhClientLoad		, int32_t clientId, uint32_t * payloadSize, byte_t * payloadBytes, uint64_t operationId)																	;
GPK_DECLARE_MODULE_FUNCTION(dwhClientStore		, int32_t clientId, uint32_t payloadSize, const byte_t * payloadBytes, uint64_t * operationId)																;

namespace dwh 
{
#pragma pack(push, 1)
	struct SClientModule : public ::gpk::SModuleBase {
		GPK_MODULE_FUNCTION_NAME(dwhClientCreate	)		Create						= 0;
		GPK_MODULE_FUNCTION_NAME(dwhClientDestroy	)		Destroy						= 0;
		GPK_MODULE_FUNCTION_NAME(dwhClientDisconnect)		Disconnect					= 0;
		GPK_MODULE_FUNCTION_NAME(dwhClientConnect	)		Connect						= 0;
		GPK_MODULE_FUNCTION_NAME(dwhClientElevate	)		Elevate						= 0;
		GPK_MODULE_FUNCTION_NAME(dwhClientResume	)		Resume						= 0;
		GPK_MODULE_FUNCTION_NAME(dwhClientLoad		)		Load						= 0;
		GPK_MODULE_FUNCTION_NAME(dwhClientStore		)		Store						= 0;
	};
#pragma pack(pop)

	::gpk::error_t						loadClientModule					(::dwh::SClientModule& loadedModule, const ::gpk::view_const_string& moduleName);
}

#endif // DWH_CLIENT_EXPORTS_H_20181013
