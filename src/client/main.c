#include "stdafx.h"

/*Unity build*/
#include "debug.c"
#include "image.c"
#include "rendering.c"
#include "window.c"
#include "lua_client.c"
#include "client.c"


int main(int argc, char** argv) {
	jeBool success = JE_TRUE;
	jeClient client;

	success = jeClient_run(&client);

	JE_MAYBE_UNUSED(argc);
	JE_MAYBE_UNUSED(argv);

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
