#if !defined(JE_CLIENT_H)
#define JE_CLIENT_H

struct jeClient {
	struct jeWindow* window;
	struct lua_State* lua;
};
bool jeClient_run();

#endif
