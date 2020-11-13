#include "stdafx.h"
#include "client.h"
#include "debug.h"
#include "container.h"
#include "window.h"
#include "lua_client.h"

#if !defined(JE_DEFAULT_GAME_DIR)
#define JE_DEFAULT_GAME_DIR "games/game"
#endif

bool jeClient_run(struct jeClient* client, const char* gameDir) {
	JE_DEBUG("client=%p, gameDir=%s", client, gameDir);

	bool ok = true;

	client->window = NULL;
	client->lua = NULL;

	struct jeString luaMainFilename;
	ok = ok && jeString_createFormatted(&luaMainFilename, "%s/main.lua", gameDir);

	struct jeString spritesFilename;
	ok = ok && jeString_createFormatted(&spritesFilename, "%s/data/sprites.png", gameDir);

	if (ok) {
		client->window = jeWindow_create(/*startVisible*/ true, jeString_get(&spritesFilename));
	}

	ok = ok && (client->window != NULL);

	ok = ok && jeLuaClient_run(client->window, jeString_get(&luaMainFilename));

	jeWindow_destroy(client->window);

	jeString_destroy(&spritesFilename);
	jeString_destroy(&luaMainFilename);

	return ok;
}
bool jeClient_run_cli(struct jeClient* client, int argc, char** argv) {
	bool ok = true;

	const char* gameDir = JE_DEFAULT_GAME_DIR;

	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-game") == 0) {
			ok = ok && ((i + 1) < argc);

			if (ok) {
				gameDir = argv[i + 1];
			}
		}
		if (strcmp(argv[i], "-debug") == 0) {
			ok = ok && ((i + 1) < argc);
		}
	}

	return jeClient_run(client, gameDir);
}
