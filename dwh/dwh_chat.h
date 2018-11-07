#include "gpk_error.h"

#ifndef DWH_CHAT_H_0289374982734
#define DWH_CHAT_H_0289374982734


namespace dwh 
{
	struct SChatServer {};
	struct SChatClient {};

	::gpk::error_t							chatServerUpdate							(SChatServer & server);
	::gpk::error_t							chatClientUpdate							(SChatClient & client);
}

#endif // DWH_CHAT_H_0289374982734
