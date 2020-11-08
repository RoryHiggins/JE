#include "stdafx.h"
#include "debug.h"
#include "client.h"
#include "window.h"
#include "lua_client.h"


bool jeClient_run(jeClient* client, const char* gameDir) {
	JE_DEBUG("client=%p, gameDir=%s", client, gameDir);

	bool ok = true;

	client->window = NULL;
	client->lua = NULL;

	jeHeapString luaMainFilename;
	ok = ok && jeHeapString_createFormatted(&luaMainFilename, "%s/main.lua", gameDir);

	jeHeapString spritesFilename;
	ok = ok && jeHeapString_createFormatted(&spritesFilename, "%s/data/sprites.png", gameDir);

	if (ok) {
		client->window = jeWindow_create(/*startVisible*/ true, jeHeapString_get(&spritesFilename));
	}

	ok = ok && (client->window != NULL);

	ok = ok && jeLuaClient_run(client->window, jeHeapString_get(&luaMainFilename));

	jeWindow_destroy(client->window);

	jeHeapString_destroy(&spritesFilename);
	jeHeapString_destroy(&luaMainFilename);

	return ok;
}
