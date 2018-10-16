#include "dwh_client_exports.h"

#if defined(GPK_WINDOWS)
#	include <Windows.h>
#endif

			::gpk::error_t						dwh::loadClientModule			(::dwh::SClientModule& loadedModule, const ::gpk::view_const_string& moduleName)				{ 
	loadedModule.Handle								= GPK_LOAD_MODULE(moduleName.begin());
	ree_if(0 == loadedModule.Handle, "Cannot load module: %s.", moduleName.begin());
	loadedModule.Create								= GPK_LOAD_MODULE_FUNCTION(loadedModule.Handle, dwhClientCreate			);
	loadedModule.Destroy							= GPK_LOAD_MODULE_FUNCTION(loadedModule.Handle, dwhClientDestroy		);
	loadedModule.Disconnect							= GPK_LOAD_MODULE_FUNCTION(loadedModule.Handle, dwhClientDisconnect		);
	loadedModule.Connect							= GPK_LOAD_MODULE_FUNCTION(loadedModule.Handle, dwhClientConnect		);
	loadedModule.Elevate							= GPK_LOAD_MODULE_FUNCTION(loadedModule.Handle, dwhClientElevate		);
	loadedModule.Resume								= GPK_LOAD_MODULE_FUNCTION(loadedModule.Handle, dwhClientResume			);
	loadedModule.Load								= GPK_LOAD_MODULE_FUNCTION(loadedModule.Handle, dwhClientLoad			);
	loadedModule.Store								= GPK_LOAD_MODULE_FUNCTION(loadedModule.Handle, dwhClientStore			);
	return 0; 
}