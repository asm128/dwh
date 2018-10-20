#include "gpk_udp_server.h"

#ifndef DWH_STORAGE_SERVER_20181017
#define DWH_STORAGE_SERVER_20181017

namespace dwh
{
	enum DWH_STORAGE_COMMAND : uint8_t
		{ DWH_STORAGE_COMMAND_STORE_NOP		= 0
		, DWH_STORAGE_COMMAND_STORE_BEGIN	= 1
		, DWH_STORAGE_COMMAND_STORE			= 2
		, DWH_STORAGE_COMMAND_STORE_END		= 3
		, DWH_STORAGE_COMMAND_LOAD_BEGIN	= 4
		, DWH_STORAGE_COMMAND_LOAD			= 5
		, DWH_STORAGE_COMMAND_LOAD_END		= 6
		};

	struct SDWHServerStorage {
		::gpk::SUDPServer									NetworkServer;
	};

	::gpk::error_t										serverStop									(::dwh::SDWHServerStorage& instance);
	::gpk::error_t										serverStart									(::dwh::SDWHServerStorage& instance, uint16_t port);
	::gpk::error_t										serverUpdate								(::dwh::SDWHServerStorage& instance);

} // namespace

#endif // DWH_STORAGE_SERVER_20181017
