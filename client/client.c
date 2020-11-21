#include "stdafx.h"

#if !defined(JE_DEFAULT_GAME_DIR)
#define JE_DEFAULT_GAME_DIR "games/game"
#endif

jeBool jeClient_run(struct jeClient* client, int argumentCount, char** arguments) {
	jeBool ok = true;

	client->window = NULL;
	client->lua = NULL;

	const char* gameDir = JE_DEFAULT_GAME_DIR;

	for (int i = 0; i < argumentCount; i++) {
		if (strcmp(arguments[i], "-game") == 0) {
			ok = ok && ((i + 1) < argumentCount);

			if (ok) {
				gameDir = arguments[i + 1];
			}
		}
		if (strcmp(arguments[i], "-debug") == 0) {
			ok = ok && ((i + 1) < argumentCount);
		}
	}

	JE_DEBUG("client=%p, gameDir=%s", client, gameDir);

	struct jeString luaMainFilename;
	ok = ok && jeString_createFormatted(&luaMainFilename, "%s/main.lua", gameDir);

	struct jeString spritesFilename;
	ok = ok && jeString_createFormatted(&spritesFilename, "%s/data/sprites.png", gameDir);

	if (ok) {
		client->window = jeWindow_create(/*startVisible*/ true, jeString_get(&spritesFilename));
	}

	ok = ok && (client->window != NULL);

	ok = ok && jeLua_run(client->window, jeString_get(&luaMainFilename), argumentCount, arguments);

	jeWindow_destroy(client->window);

	jeString_destroy(&spritesFilename);
	jeString_destroy(&luaMainFilename);

	return ok;
}
