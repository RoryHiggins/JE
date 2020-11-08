#if !defined(JE_CLIENT_H)
#define JE_CLIENT_H

#include "lua_client.h"


typedef struct jeClient jeClient;
struct jeClient {
	struct jeWindow* window;
	struct lua_State* lua;
};
bool jeClient_run();

#endif
