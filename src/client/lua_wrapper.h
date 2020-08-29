#if !defined(JE_LUA_WRAPPER_H)
#define JE_LUA_WRAPPER_H

#include "stdafx.h"


const char* jeLua_getError(lua_State* lua);
jeBool jeLuaClient_registerLuaClientBindings(lua_State* lua);

#endif
