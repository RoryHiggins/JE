#include "stdafx.h"
#include "debug.h"
#include "client.h"

/*Unity build*/
#include "debug.c"
#include "container.c"
#include "image.c"
#include "rendering.c"
#include "window.c"
#include "lua_client.c"
#include "client.c"

int main(int argc, char** argv) {
	struct jeClient client;
	bool ok = jeClient_run_cli(&client, argc, argv);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
