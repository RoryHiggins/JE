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

	ok = ok && jeClient_run(&client, gameDir);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
