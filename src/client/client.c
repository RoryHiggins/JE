#include "stdafx.h"
#include "debug.h"
#include "client.h"
#include "window.h"
#include "lua_client.h"

#define JE_CLIENT_LUA_MAIN_FILENAME "src/game/main.lua"


typedef struct jeClient jeClient;

struct jeClient {
	lua_State* lua;
};

void jeClient_destroy(jeClient* client) {
	JE_DEBUG("jeClient_destroy()");

	if (client->lua != NULL) {
		lua_close(client->lua);
		client->lua = NULL;
	}

	jeWindow_destroy(jeWindow_get());

	memset((void*)client, 0, sizeof(*client));
}
jeBool jeClient_create(jeClient* client) {
	jeBool success = JE_FALSE;

	JE_DEBUG("jeClient_create()");

	memset((void*)client, 0, sizeof(*client));

	if (jeWindow_create(jeWindow_get()) == JE_FALSE) {
		JE_ERROR("jeClient_create(): jeWindow_create() failed");
		goto finalize;
	}

	client->lua = luaL_newstate();

	if (client->lua == NULL) {
		JE_ERROR("jeClient_create(): luaL_newstate() failed");
		goto finalize;
	}

	luaL_openlibs(client->lua);

	if (jeLuaClient_registerLuaClientBindings(client->lua) == JE_FALSE) {
		JE_ERROR("jeClient_create(): jeLuaClient_registerLuaClientBindings() failed");
		goto finalize;
	}

	success = JE_TRUE;
	finalize: {
	}

	return success;
}
jeBool jeClient_run() {
	jeBool success = JE_FALSE;
	int luaResponse = 0;
	jeClient client;

	memset((void*)&client, 0, sizeof(client));

	JE_INFO("jeClient_run()");

	if (jeClient_create(&client) == JE_FALSE) {
		JE_ERROR("jeClient_run(): jeClient_create() failed");
		goto finalize;
	}

	luaResponse = luaL_loadfile(client.lua, JE_CLIENT_LUA_MAIN_FILENAME);

	if (luaResponse != 0) {
		JE_ERROR("jeClient_run(): luaL_loadfile() failed, filename=%s luaResponse=%d error=%s", JE_CLIENT_LUA_MAIN_FILENAME, luaResponse, jeLuaClient_getError(client.lua));
		goto finalize;
	}

	luaResponse = lua_pcall(client.lua, /* num args */ 0, /* num return vals */ LUA_MULTRET, /* err func */ 0);

	if (luaResponse != 0) {
		JE_ERROR("jeClient_run(): lua_pcall() failed, filename=%s luaResponse=%d error=%s", JE_CLIENT_LUA_MAIN_FILENAME, luaResponse, jeLuaClient_getError(client.lua));
		goto finalize;
	}

	success = JE_TRUE;
	finalize: {
		jeClient_destroy(&client);
	}

	return success;
}
