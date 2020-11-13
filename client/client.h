#if !defined(JE_CLIENT_H)
#define JE_CLIENT_H

struct jeClient {
	struct jeWindow* window;
	struct lua_State* lua;
};
bool jeClient_run(struct jeClient* client, const char* gameDir);
bool jeClient_run_cli(struct jeClient* client, int argc, char** argv);

#endif
