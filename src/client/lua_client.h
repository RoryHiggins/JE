#if !defined(JE_LUA_CLIENT_H)
#define JE_LUA_CLIENT_H

#include "stdafx.h"


const char* jeLuaClient_getError(lua_State* lua);
void jeLuaClient_updateStates(lua_State* lua);
jeBool jeLuaClient_registerLuaClientBindings(lua_State* lua);

#endif
