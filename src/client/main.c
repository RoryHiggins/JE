#include "stdafx.h"
#include "logging.h"
#include "rendering.h"
#include "window.h"
#include "lua.h"
#include "client.h"

int main(int argc, char** argv) {
	bool success = true;
	jeClient game;

	JE_MAYBE_UNUSED(argc);
	JE_MAYBE_UNUSED(argv);

	success = jeClient_run(&game);

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
