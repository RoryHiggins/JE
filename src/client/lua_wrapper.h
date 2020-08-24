#include "stdafx.h"

#if !defined(JE_LUA_WRAPPER_H)
#define JE_LUA_WRAPPER_H

#include "core.h"


const char* jeLua_getError(lua_State* lua);
bool jeLuaClient_registerLuaClientBindings(lua_State* lua);

#endif
