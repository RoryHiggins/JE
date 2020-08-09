#pragma once
#include "stdafx.h"
#include "logging.h"
#include "rendering.h"
#include "window.h"
#include "lua.h"

#define JE_CLIENT_LUA_MAIN_FILENAME "src/game/main.lua"

typedef struct {
	lua_State* lua;
} jeClient;
bool jeClient_create(jeClient* client) {
	bool success = false;

	JE_LOG("jeClient_create()");

	client->lua = NULL;

	if (!jeWindow_create(jeWindow_get())) {
		goto cleanup;
	}

	client->lua = luaL_newstate();
	if (client->lua == NULL) {
		JE_ERR("jeClient_create(): luaL_newstate() failed");
		goto cleanup;
	}

	luaL_openlibs(client->lua);

	if (!jeLuaClient_registerLuaClientBindings(client->lua)) {
		goto cleanup;
	}

	success = true;
	cleanup: {
	}
	return success;
}
void jeClient_destroy(jeClient* client) {
	if (client->lua != NULL) {
		lua_close(client->lua);
		client->lua = NULL;
	}

	jeWindow_destroy(jeWindow_get());
}
bool jeClient_run(jeClient* client) {
	bool success = false;
	int response = 0;

	if (!jeClient_create(client)) {
		goto cleanup;
	}

	response = luaL_loadfile(client->lua, JE_CLIENT_LUA_MAIN_FILENAME);
	if (response != 0) {
		JE_ERR("jeClient_create(): luaL_loadfile() failed, filename=%s response=%d error=%s", JE_CLIENT_LUA_MAIN_FILENAME, response, jeLua_getError(client->lua));
		goto cleanup;
	}

	response = lua_pcall(client->lua, /* num args */ 0, /* num return vals */ LUA_MULTRET, /* err func */ 0);
	if (response != 0) {
		JE_ERR("jeClient_create(): lua_pcall() failed, filename=%s response=%d error=%s", JE_CLIENT_LUA_MAIN_FILENAME, response, jeLua_getError(client->lua));
		goto cleanup;
	}

	success = true;
	cleanup: {
		jeClient_destroy(client);
	}

	return success;
}
