#include "stdafx.h"

#if !defined(JE_CLIENT_H)
#define JE_CLIENT_H

#include "core.h"


typedef struct {
	lua_State* lua;
} jeClient;

bool jeClient_run();

#endif
