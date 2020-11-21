#if !defined(JE_LUA_CLIENT_H)
#define JE_LUA_CLIENT_H

#include "common.h"

struct jeWindow;

/* Runs the given lua script, blocking until completion */
JE_PUBLIC jeBool jeLua_run(struct jeWindow* window, const char* filename, int argumentCount, char** arguments);

#endif
