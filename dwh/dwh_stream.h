#include "gpk_coord.h"
#include "gpk_array.h"

#ifndef DWH_STREAM_H_2930840293
#define DWH_STREAM_H_2930840293

namespace dwh
{
#pragma pack(push, 1)
	struct SLineFormat {
		uint8_t							Bits16				: 1;
		uint8_t							ChannelB			: 1;
		uint8_t							ChannelG			: 1;
		uint8_t							ChannelR			: 1;
		uint8_t							ChannelA			: 1;
	};
	struct SLineHeader {
		uint8_t							SessionCommand		= 7;
		SLineFormat						Format				= {};
		::gpk::SRectangle2D<uint16_t>	Area				= {{}, {(uint16_t)-1, (uint16_t)-1}};
		uint16_t						AdditionalLines		= 0;
		::gpk::SCoord2<uint16_t>		ImageSize			= {};
	};
#pragma pack(pop)
}

#endif // DWH_STREAM_H_2930840293
