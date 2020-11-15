#if !defined(JE_LUA_CLIENT_H)
#define JE_LUA_CLIENT_H

/* Run the given lua script, blocking until completion */
bool jeLua_run(struct jeWindow* window, const char* filename, int argc, char** argv);

#endif
