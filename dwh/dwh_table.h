#include "dwh_datatype.h"

#include "gpk_array.h"
#include "gpk_label.h"

#ifndef DWH_TABLE_H_0289347902834
#define DWH_TABLE_H_0289347902834


namespace dwh
{
	struct STable {
		::gpk::array_pod<char_t>					TableName;
		::gpk::array_pod<::dwh::SDataTypeID>		FieldTypes;
		::gpk::array_pod<::gpk::label>				FieldNames;
	};

	//struct SDBState {
	//	::gpk::array_pod<>
	//};

	::gpk::error_t								tableOpen							(::dwh::STable &table, const ::gpk::view_const_string& tableName);
	::gpk::error_t								tableClose							(::dwh::STable &table, const ::gpk::view_const_string& tableName);
}

#endif // DWH_TABLE_H_0289347902834
