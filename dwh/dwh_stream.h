#include "gpk_coord.h"
#include "gpk_array.h"

#ifndef DWH_STREAM_H_2930840293
#define DWH_STREAM_H_2930840293

namespace dwh
{
#pragma pack(push, 1)
	struct SLineFormat {
		uint8_t							Bits16		: 1;
		uint8_t							Compression	: 1;
	};
	struct SLineHeader {
		uint8_t							SessionCommand	= 7;
		SLineFormat						Format			= {};
		uint16_t						LineNumber		= 0;			
		::gpk::SCoord2<uint16_t>		ImageSize		= {};
	};
#pragma pack(pop)

	::gpk::error_t									lineDeflate								(const ::gpk::view_array<const ubyte_t>	& inflated, ::gpk::array_pod<ubyte_t>& deflated);
	::gpk::error_t									lineInflate								(const ::gpk::view_array<ubyte_t>		& deflated, ::gpk::array_pod<ubyte_t>& inflated);
}

#endif // DWH_STREAM_H_2930840293
