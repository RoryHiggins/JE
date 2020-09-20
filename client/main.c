#include "stdafx.h"

/*Unity build*/
#include "debug.c"
#include "container.c"
#include "image.c"
#include "rendering.c"
#include "window.c"
#include "lua_client.c"
#include "client.c"


int main(int argc, char** argv) {
	bool ok = true;

	jeClient client;

	const char* gameDir = "games/game";
	if (argc > 1) {
		gameDir = argv[1];
	}

	ok = ok && jeClient_run(&client, gameDir);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
