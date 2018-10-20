#include "dwh_datatype.h"

#include "gpk_array.h"
#include "gpk_label.h"
#include "gpk_ptr.h"

#ifndef GPK_DB_H_20181018
#define GPK_DB_H_20181018

namespace dwh
{
#pragma pack(push, 1)
	struct SDBTable {
							::gpk::label											Name;
							::gpk::array_obj<::gpk::label>							FieldNames;
							::gpk::array_pod<::dwh::DATA_TYPE>						FieldDescriptors;
	};

	struct SDBAccessRights {
							bool													RecordLoad					: 1;
							bool													RecordStore					: 1;
							bool													RecordAdd					: 1;
							bool													RecordDelete				: 1;
							bool													FieldCreate					: 1;
							bool													FieldDelete					: 1;
							bool													TableDelete					: 1;
	};

	struct SDBTableAccessRights {		
							int32_t													IdTable						= -1;
							::dwh::SDBAccessRights									Rights						= {1, };
	};
		
#pragma pack(pop)		
	struct SDBUserAccessRights {		
							::gpk::array_obj<::dwh::SDBTableAccessRights>			TableRights;
	};
		
	struct SDB {		
							::gpk::label											Name						= {};
							::gpk::array_obj<::gpk::label>							TableNames					= {};		// used for dbRecordLoad outputBytes parameter
							::gpk::array_obj<::gpk::label>							UserNames					= {};		// used for dbRecordLoad outputBytes parameter
							::gpk::array_obj<::dwh::SDBUserAccessRights>			UserAccessRights			= {};
							
							::gpk::array_obj<::gpk::ptr_obj<::dwh::SDBTable>>		TablesLoaded;

							::gpk::array_pod<byte_t>								CacheRecord					= {};	// used for dbRecordLoad outputBytes parameter
	};

						::gpk::error_t											dbSave						(SDB& db, const ::gpk::view_const_string& path);
						::gpk::error_t											dbOpen						(SDB& db, const ::gpk::view_const_string& path, const ::gpk::view_const_string& name);
	
						::gpk::error_t											dbTableDelete				(SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName);
						::gpk::error_t											dbTableCreate				(SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName);
						::gpk::error_t											dbFieldDelete				(SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName);
						::gpk::error_t											dbFieldCreate				(SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, const ::dwh::DATA_TYPE dataType);
						::gpk::error_t											dbRecordPop					(SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, ::gpk::array_pod<byte_t>& outputBytes);
						::gpk::error_t											dbRecordPush				(SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, const ::gpk::array_pod<byte_t>& inputBytes);
						::gpk::error_t											dbRecordLoad				(SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, uint64_t elementIndex, ::gpk::array_pod<byte_t>& outputBytes);
						::gpk::error_t											dbRecordStore				(SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, uint64_t elementIndex, const ::gpk::array_pod<byte_t>& inputBytes);
}

#endif
