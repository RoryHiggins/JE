#include "stdafx.h"
#include "debug.c"
#include "container.c"
#include "client.c"

int main(int argc, char** argv) {
	jeBool ok = true;
	struct jeClient client;
	ok = ok && jeClient_run(&client, argc, argv);

	JE_MAYBE_UNUSED(argc);
	JE_MAYBE_UNUSED(argv);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
