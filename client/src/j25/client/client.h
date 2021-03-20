#pragma once

#if !defined(JE_CORE_CLIENT_H)
#define JE_CORE_CLIENT_H

#include <j25/core/api.h>

#include <stdbool.h>


struct jeWindow;
struct lua_State;


struct jeClient {
	struct jeWindow* window;
	struct lua_State* lua;
};
bool jeClient_run(struct jeClient* client, int argumentCount, char** arguments);

#endif
