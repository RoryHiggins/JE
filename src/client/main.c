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

	ok = ok && jeClient_run(&client);

	JE_MAYBE_UNUSED(argc);
	JE_MAYBE_UNUSED(argv);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
