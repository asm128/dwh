#include "dwh_db.h"

#include "gpk_view_stream.h"
#include "gpk_storage.h"
#include "gpk_encoding.h"

//#define DATABASE_FILE_EXTENSION		"dbh"

static	::gpk::error_t					appendLabelList					(::gpk::array_pod<byte_t>& output, const ::gpk::array_obj<::gpk::label> & labels)				{ 
	uint32_t									countLabels						= labels.size();					
	gpk_necall(output.append((const byte_t*)&countLabels, sizeof(uint32_t)), "Out of memory? output size. %u.", output.size());
	for(uint32_t iLabel = 0; iLabel < countLabels; ++iLabel) {
		const ::gpk::view_const_string				label							= labels[iLabel];
		uint16_t									labelLen						= (uint16_t)label.size();
		gpk_necall(output.append((const byte_t*)&labelLen, sizeof(uint16_t)), "Out of memory? output size. %u.", output.size());
		gpk_necall(output.append(label.begin(), labelLen), "Out of memory? output size. %u.", output.size());
	}
	return 0;
}

static	::gpk::error_t					readLabelList					(::gpk::view_stream<byte_t>& fileStream, ::gpk::array_obj<::gpk::label>& labels)				{ //(::gpk::array_obj<::gpk::label>& labels, ::gpk::view_stream<byte_t>& fileStream)																															{ 
	uint32_t									countLabels						= 0;							
	gpk_necall(fileStream.read_pod(countLabels), "Invalid stream size: %u.", fileStream.size());
	const uint32_t								labelOffset						= labels.size();
	gpk_necall(labels.resize(labelOffset + countLabels), "Out of memory? Label count: %u.", countLabels);
	for(uint32_t iLabel = labelOffset, labelMax = labelOffset + countLabels; iLabel < labelMax; ++iLabel) {
		uint16_t									labelLen						= 0;
		gpk_necall(fileStream.read_pod(labelLen), "Invalid stream size: %u.", fileStream.size());
		labels[iLabel]							= {&fileStream[fileStream.CursorPosition], labelLen};
		fileStream.CursorPosition				+= labelLen;
	}
	return 0;
}

static	::gpk::error_t					dbFileName						(const ::gpk::view_const_string& name, const ::gpk::view_const_string& path, char_t* fileName, uint32_t* nameLen)																										{ 
	::gpk::array_pod<byte_t>					hexedDBName						= {};	// Store to disk
	::gpk::hexEncode({(const ubyte_t*)name.begin(), name.size()}, hexedDBName);
	hexedDBName.push_back(0);
	static constexpr	const char				pathFormat	[]					= "%s\\db_%s.dbi";
	if(0 == nameLen) 
		return sprintf_s(fileName, 240, pathFormat, path.begin(), hexedDBName.begin());
	else
		*nameLen								= (uint32_t)sprintf_s(fileName, 256, pathFormat, path.begin(), hexedDBName.begin());
		if((int32_t)*nameLen == -1) {
			*nameLen								= 0;
			return -1;
		}
	return 0;
}

		::gpk::error_t					dwh::dbSave						(::dwh::SDB& db, const ::gpk::view_const_string& path)																												{ 
	::gpk::array_pod<byte_t>					output							= {};	// Serialize
	gpk_necall(output.append((const byte_t*)&db.Name.size(), sizeof(uint32_t)	), "Failed to serialize. Invalid db info size? %u.", output.size());
	gpk_necall(output.append(db.Name.begin(), db.Name.size()					), "Failed to serialize. Invalid db info size? %u.", output.size());
	gpk_necall(::appendLabelList(output, db.TableNames							), "Failed to serialize. Invalid db info size? %u.", output.size());
	gpk_necall(::appendLabelList(output, db.UserNames							), "Failed to serialize. Invalid db info size? %u.", output.size());
	for(uint32_t iUser = 0; iUser < db.UserNames.size(); ++iUser) {
		output.append((const byte_t*)&db.UserAccessRights[iUser].TableRights.size(), sizeof(uint32_t));
		gpk_necall(output.append((const byte_t*)db.UserAccessRights[iUser].TableRights.begin(), db.UserAccessRights[iUser].TableRights.size() * sizeof(::dwh::SDBTableAccessRights)), "Failed to serialize. Invalid db info size? %u.", output.size());
	}
	char_t										fileName[512]					= {};	// Store to disk
	uint32_t									nameLen							= 0;
	gpk_necall(::dbFileName(db.Name, path, fileName, &nameLen), "Path too long.");
	info_printf("Storing database state to '%s'.", fileName);
	gpk_necall(::gpk::fileFromMemory(fileName, output), "Failed to store db file : '%s'.", fileName);
	return 0;
}

		::gpk::error_t					dwh::dbOpen						(::dwh::SDB& db, const ::gpk::view_const_string& path, const ::gpk::view_const_string& name)																												{ 
	::gpk::array_pod<byte_t>					fileInMemory					= {};	// Read from disk
	char_t										fileName[512]					= {};	// Store to disk
	uint32_t									nameLen							= 0;
	gpk_necall(::dbFileName(name, path, fileName, &nameLen), "Path too long.");
	info_printf("Opening database state from '%s'.", fileName);
	gpk_necall(::gpk::fileToMemory(fileName, fileInMemory), "Failed to read db file! '%s'.", fileName);

	::gpk::view_stream<byte_t>					fileStream						= {fileInMemory.begin(), fileInMemory.size()};	// Deserialize
	nameLen									= 0;
	gpk_necall(fileStream.read_pod(nameLen), "Invalid db info size: %u.", fileStream.size());
	db.Name									= {&fileStream[fileStream.CursorPosition], nameLen};
	fileStream.CursorPosition				+= nameLen;
	gpk_necall(::readLabelList(fileStream, db.TableNames	), "Failed to load user names. Invalid db info size? %u.", fileStream.size());
	gpk_necall(::readLabelList(fileStream, db.UserNames		), "Failed to load user names. Invalid db info size? %u.", fileStream.size());

	gpk_necall(db.UserAccessRights.resize(db.UserNames.size()), "Out of memory? User count: %u.", db.UserNames.size()); 
	for(uint32_t iUser = 0; iUser < db.UserNames.size(); ++iUser) {
		uint32_t									tableCount						= 0;
		gpk_necall(fileStream.read_pod(tableCount), "Invalid db info size: %u.", fileStream.size());
		::gpk::array_obj<SDBTableAccessRights>		& tableRights					= db.UserAccessRights[iUser].TableRights;
		gpk_necall(tableRights.resize(tableCount), "Out of memory? Table count: %u.", tableCount); 
		gpk_necall(fileStream.read_pod(tableRights.begin(), tableRights.size()), "Invalid db info size: %u.", fileStream.size());
	}
	info_printf("Succeeded loading database: %s.", db.Name.begin());
	return 0; 
}

		::gpk::error_t					getUserIndex					(::dwh::SDB& db, const ::gpk::view_const_string& userName)		{
	for(int32_t iUser = 0, countUsers = (int32_t)db.UserNames.size(); iUser < countUsers; ++iUser)		
		if(0 == strcmp(userName.begin(), db.UserNames[iUser].begin()))
			return iUser;
	return -1;
}

		::gpk::error_t					dwh::dbTableCreate				(::dwh::SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName)																													{ 
	int32_t										iUser							= getUserIndex(db, userName);
	ree_if(errored(iUser), "User doesn't exist: %s.", userName.begin());

	for(uint32_t iTable = 0; iTable < db.TableNames.size(); ++iTable) 
		ree_if(0 == strcmp(tableName.begin(), db.TableNames[iTable].begin()), "Table already exists: %s.", tableName.begin());

	int32_t										tableIndex						= -1;
	gpk_necall(tableIndex = db.TableNames.push_back(tableName), "Failed to add table: %s. Out of memory? Table count: %u.", tableName.begin(), db.TableNames.size());
	::dwh::SDBUserAccessRights					& adminRights					= db.UserAccessRights[iUser];
	gpk_necall(adminRights.TableRights.push_back({tableIndex, {1, 1, 1, 1, 1, 1, 1}}), "Failed to add table: %s. Out of memory? Table count: %u.", tableName.begin(), db.TableNames.size());;	// User has full access to the table it created.
	return 0; 
}

		::gpk::error_t					dwh::dbTableDelete				(::dwh::SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName)																												{ 
	int32_t										iUser							= getUserIndex(db, userName);
	ree_if(errored(iUser), "User doesn't exist: %s.", userName.begin());
	db, userName, tableName											; 
	return 0; 
}

		::gpk::error_t					dwh::dbFieldCreate				(::dwh::SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, const ::dwh::DATA_TYPE dataType)									{ 
	int32_t										iUser							= getUserIndex(db, userName);
	ree_if(errored(iUser), "User doesn't exist: %s.", userName.begin());
	db, userName, tableName, fieldName, dataType					; 
	return 0; 
}

		::gpk::error_t					dwh::dbFieldDelete				(::dwh::SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName)																	{ 
	int32_t										iUser							= getUserIndex(db, userName);
	ree_if(errored(iUser), "User doesn't exist: %s.", userName.begin());
	db, userName, tableName, fieldName								; 
	return 0; 
}

		::gpk::error_t					dwh::dbRecordLoad				(::dwh::SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, uint64_t elementIndex, ::gpk::array_pod<byte_t>& outputBytes)		{ 
	int32_t										iUser							= getUserIndex(db, userName);
	ree_if(errored(iUser), "User doesn't exist: %s.", userName.begin());
	db, userName, tableName, fieldName, elementIndex, outputBytes	; 
	return 0; 
}

		::gpk::error_t					dwh::dbRecordStore				(::dwh::SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, uint64_t elementIndex, const ::gpk::array_pod<byte_t>& inputBytes)	{ 
	int32_t										iUser							= getUserIndex(db, userName);
	ree_if(errored(iUser), "User doesn't exist: %s.", userName.begin());
	db, userName, tableName, fieldName, elementIndex, inputBytes	; 
	return 0; 
}

		::gpk::error_t					dwh::dbRecordPop				(::dwh::SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, ::gpk::array_pod<byte_t>& outputBytes)								{ 
	int32_t										iUser							= getUserIndex(db, userName);
	ree_if(errored(iUser), "User doesn't exist: %s.", userName.begin());
	db, userName, tableName, fieldName, outputBytes					; 
	return 0; 
}

		::gpk::error_t					dwh::dbRecordPush				(::dwh::SDB& db, const ::gpk::view_const_string& userName, const ::gpk::view_const_string& tableName, const ::gpk::view_const_string& fieldName, const ::gpk::array_pod<byte_t>& inputBytes)						{ 
	int32_t										iUser							= getUserIndex(db, userName);
	ree_if(errored(iUser), "User doesn't exist: %s.", userName.begin());
	db, userName, tableName, fieldName, inputBytes					; 
	return 0; 
}

