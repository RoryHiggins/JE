#if !defined(JE_LUA_CLIENT_H)
#define JE_LUA_CLIENT_H

#include "stdafx.h"
#include "window.h"


typedef struct jeLuaClient jeLuaClient;

struct jeLuaClient {
	lua_State* lua;
};

jeBool jeLuaClient_run(jeLuaClient* luaClient, jeWindow* window, const char* filename);

#endif
