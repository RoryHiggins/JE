#if !defined(JE_CLIENT_H)
#define JE_CLIENT_H

#include "stdafx.h"
#include "lua_client.h"


typedef struct jeClient jeClient;

struct jeClient {
	jeWindow* window;
	jeLuaClient luaClient;
};

jeBool jeClient_run();

#endif
