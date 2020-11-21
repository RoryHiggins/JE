/*Unity build*/
#include "stdafx.h"
#include "debug.c"
#include "container.c"
#include "image.c"
#include "rendering.c"
#include "window.c"
#include "lua_client.c"
#include "client.c"

int main(int argc, char** argv) {
	struct jeClient client;
	jeBool ok = jeClient_run(&client, argc, argv);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
