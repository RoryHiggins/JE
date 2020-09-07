#include "stdafx.h"
#include "debug.h"
#include "client.h"
#include "window.h"
#include "lua_client.h"

#define JE_CLIENT_LUA_MAIN_FILENAME "src/game/main.lua"


void jeClient_destroy(jeClient* client) {
	JE_DEBUG("jeClient_destroy()");

	jeWindow_destroy(client->window);

	memset((void*)client, 0, sizeof(*client));
}
jeBool jeClient_create(jeClient* client) {
	jeBool success = JE_FALSE;

	JE_DEBUG("jeClient_create()");

	memset((void*)client, 0, sizeof(*client));

	client->window = jeWindow_create();
	if (client->window == NULL) {
		JE_ERROR("jeClient_create(): jeWindow_create() failed");
		goto finalize;
	}

	success = JE_TRUE;
	finalize: {
	}

	return success;
}
jeBool jeClient_run(jeClient* client) {
	jeBool success = JE_FALSE;

	memset((void*)client, 0, sizeof(*client));

	JE_INFO("jeClient_run()");

	if (jeClient_create(client) == JE_FALSE) {
		JE_ERROR("jeClient_run(): jeClient_create() failed");
		goto finalize;
	}

	if (jeLuaClient_run(&client->luaClient, client->window, JE_CLIENT_LUA_MAIN_FILENAME) == JE_FALSE) {
		JE_ERROR("jeClient_run(): jeLuaClient_run() ended abnormally");
		goto finalize;
	}

	success = JE_TRUE;
	finalize: {
		jeClient_destroy(client);
	}

	return success;
}
