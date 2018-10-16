#include <cstdint>

#ifndef DWH_COMMAND_H_20181013
#define DWH_COMMAND_H_20181013

namespace dwh
{
#pragma pack(push, 1)
	enum DWH_COMMAND : uint8_t
		{ DWH_COMMAND_NOP				= 0	// - Do nothing
		, DWH_COMMAND_CONNECT			= 1	// - Joins the network - Gets a client id and guest session token.
		, DWH_COMMAND_LOAD				= 2	// - Requests data from the service. - Payload 0: Request result size and parts size. - Payload 1: Request data part. - Payload 2: Undefined. - Payload 3: Undefined.
		, DWH_COMMAND_STORE				= 3	// - Sends data to the service. - Payload 0: Request storage space and report part. - Payload 1: Send data part. - Payload 2: Undefined. - Payload 3: Undefined.
		, DWH_COMMAND_RESERVED_3		= 4	// - Undefined
		, DWH_COMMAND_RESERVED_2		= 5	// - Undefined
		, DWH_COMMAND_RESERVED_1		= 6	// - Undefined
		, DWH_COMMAND_RESERVED_0		= 7	// - Undefined
		};

	enum DWH_COMMAND_TYPE : uint8_t		
		{ DWH_COMMAND_TYPE_REQUEST		= 0
		, DWH_COMMAND_TYPE_RESPONSE		= 1
		};

	struct SDWHCommand {
		DWH_COMMAND						Command				: 3;
		DWH_COMMAND_TYPE				Type				: 1;
		uint8_t							Payload				: 4;
	};

	struct SDWHPayloadHeader {
		SDWHCommand						Command				= {};
		uint8_t							Reserved0			= 0;
		uint16_t						Reserved1			= 0;
		uint32_t						PayloadSize			= 0;
		uint64_t						PayloadHash			= 0;
	};

	static constexpr const size_t	as0					= sizeof(SDWHPayloadHeader);

#pragma pack(pop)
} // namespace

#endif // DWH_COMMAND_H_20181013
