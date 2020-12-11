#if !defined(JE_CLIENT_CLIENT_H)
#define JE_CLIENT_CLIENT_H

#include <j25/stdafx.h>

struct jeWindow;
struct lua_State;

struct jeClient {
	struct jeWindow* window;
	struct lua_State* lua;
};

JE_PUBLIC bool jeClient_run(struct jeClient* client, int argumentCount, char** arguments);

#endif
