#include "dwh_db.h"

int													main						()							{
	static constexpr	const char							dbName		[]				= "My Database";
	static constexpr	const char							tableName	[]				= "null table";
	static constexpr	const char							adminName	[]				= "admin";
	{
		::dwh::SDB												db							= {};
		db.Name												= dbName;
		db.UserNames		.push_back(adminName);
		db.UserAccessRights	.push_back({});

		gpk_necall(::dwh::dbTableCreate(db, adminName, tableName), "Failed to create table: %s.", tableName);
		i_if(errored(::dwh::dbTableCreate(db, adminName, tableName)), "Failed to create table: %s. This is ok because it already exists.", tableName);
		e_if(errored(::dwh::dbTableCreate(db, adminName, "Table 0")), "Failed to create table: %s. This is ok because it already exists.", tableName);
		e_if(errored(::dwh::dbTableCreate(db, adminName, "Table 1")), "Failed to create table: %s. This is ok because it already exists.", tableName);
		e_if(errored(::dwh::dbTableCreate(db, adminName, "Table 2")), "Failed to create table: %s. This is ok because it already exists.", tableName);
		gpk_necall(::dwh::dbSave(db, "C:"), "Failed to save database state: %s.", dbName);
	}
	{
		::dwh::SDB												db							= {};
		gpk_necall(::dwh::dbOpen(db, "C:", dbName), "Failed to open database: %s.", dbName);
		for(uint32_t iTable = 0; iTable < db.TableNames.size(); ++iTable) 
			always_printf("Table %u: %s.", iTable, db.TableNames[iTable].begin());

		for(uint32_t iUser = 0; iUser < db.UserNames.size(); ++iUser) {
			always_printf("Name: %s.", db.UserNames[iUser].begin());
			for(uint32_t iTableRights = 0; iTableRights < db.UserAccessRights[iUser].TableRights.size(); ++iTableRights)
				always_printf("Permissions: "
					"\n-- Table index -- : %i."
					"\nRecordLoad        : %u."
					"\nRecordStore       : %u."
					"\nRecordAdd         : %u."
					"\nRecordDelete      : %u."
					"\nFieldCreate       : %u."
					"\nFieldDelete       : %u."
					"\nTableDelete       : %u."
					, (int32_t )db.UserAccessRights[iUser].TableRights[iTableRights].IdTable
					, (uint32_t)db.UserAccessRights[iUser].TableRights[iTableRights].Rights.RecordLoad		
					, (uint32_t)db.UserAccessRights[iUser].TableRights[iTableRights].Rights.RecordStore		
					, (uint32_t)db.UserAccessRights[iUser].TableRights[iTableRights].Rights.RecordAdd		
					, (uint32_t)db.UserAccessRights[iUser].TableRights[iTableRights].Rights.RecordDelete	
					, (uint32_t)db.UserAccessRights[iUser].TableRights[iTableRights].Rights.FieldCreate		
					, (uint32_t)db.UserAccessRights[iUser].TableRights[iTableRights].Rights.FieldDelete		
					, (uint32_t)db.UserAccessRights[iUser].TableRights[iTableRights].Rights.TableDelete		
					);	
			}
	}
	return 0;
}