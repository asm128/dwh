#include "dwh_client.h"

::gpk::error_t	GPK_STDCALL			dwhClientCreate					()																																						{ ; return 0; }
::gpk::error_t	GPK_STDCALL			dwhClientDestroy				(int32_t clientId)																																		{ clientId																		; return 0; }
::gpk::error_t	GPK_STDCALL			dwhClientDisconnect				(int32_t clientId)																																		{ clientId																		; return 0; }
::gpk::error_t	GPK_STDCALL			dwhClientConnect				(int32_t clientId, int32_t ip0, int32_t ip1, int32_t ip2, int32_t ip3, int32_t port, uint32_t * tokenSize, byte_t * tokenBytes)							{ clientId, ip0, ip1, ip2, ip3, port, tokenSize	, tokenBytes					; return 0; }
::gpk::error_t	GPK_STDCALL			dwhClientElevate				(int32_t clientId, uint32_t userSize, const byte_t * userBytes, uint32_t passSize, const byte_t * passBytes, uint32_t * tokenSize, byte_t * tokenBytes)	{ clientId, userSize	, userBytes, passSize, passBytes, tokenSize	, tokenBytes; return 0; }
::gpk::error_t	GPK_STDCALL			dwhClientResume					(int32_t clientId, int32_t ip0, int32_t ip1, int32_t ip2, int32_t ip3, int32_t port, uint32_t tokenSize, const byte_t * tokenBytes)						{ clientId, ip0, ip1, ip2, ip3, port, tokenSize	, tokenBytes					; return 0; }
::gpk::error_t	GPK_STDCALL			dwhClientLoad					(int32_t clientId, uint32_t * payloadSize, byte_t * payloadBytes, uint64_t operationId)																	{ clientId, payloadSize	, payloadBytes, operationId								; return 0; }
::gpk::error_t	GPK_STDCALL			dwhClientStore					(int32_t clientId, uint32_t payloadSize, const byte_t * payloadBytes, uint64_t * operationId)															{ clientId, payloadSize	, payloadBytes, operationId								; return 0; }
