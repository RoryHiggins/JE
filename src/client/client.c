#include "stdafx.h"
#include "debug.h"
#include "client.h"
#include "window.h"
#include "lua_client.h"

#define JE_CLIENT_LUA_MAIN_FILENAME "src/game/main.lua"

jeBool jeClient_run(jeClient* client) {
	jeBool success = JE_FALSE;

	memset((void*)client, 0, sizeof(*client));

	JE_INFO("jeClient_run()");

	client->window = jeWindow_create();
	if (client->window == NULL) {
		JE_ERROR("jeClient_run(): jeWindow_create() failed");
		goto finalize;
	}

	if (jeLuaClient_run(client->window, JE_CLIENT_LUA_MAIN_FILENAME) == JE_FALSE) {
		JE_ERROR("jeClient_run(): jeLuaClient_run() ended abnormally");
		goto finalize;
	}

	success = JE_TRUE;
	finalize: {
		jeWindow_destroy(client->window);
	}

	return success;
}
