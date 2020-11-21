#if !defined(JE_CLIENT_H)
#define JE_CLIENT_H

#include "common.h"

struct jeClient {
	struct jeWindow* window;
	struct lua_State* lua;
};

JE_PUBLIC jeBool jeClient_run(struct jeClient* client, int argumentCount, char** arguments);

#endif
