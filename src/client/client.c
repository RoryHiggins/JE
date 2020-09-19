#include "stdafx.h"
#include "debug.h"
#include "client.h"
#include "window.h"
#include "lua_client.h"

#define JE_CLIENT_LUA_MAIN_FILENAME "src/game/main.lua"


bool jeClient_run(jeClient* client) {
	bool ok = true;

	client->window = NULL;
	client->lua = NULL;

	JE_INFO("");

	if (ok) {
		client->window = jeWindow_create();
	}

	ok = ok && (client->window != NULL);
	ok = ok && jeLuaClient_run(client->window, JE_CLIENT_LUA_MAIN_FILENAME);

	jeWindow_destroy(client->window);

	return ok;
}
