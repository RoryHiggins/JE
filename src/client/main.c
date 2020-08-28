#include "precompiled.h"
#include "core.h"
#include "window.h"
#include "lua_wrapper.h"
#include "client.h"


int main(int argc, char** argv) {
	bool success = true;

	success = jeClient_run();

	JE_MAYBE_UNUSED(argc);
	JE_MAYBE_UNUSED(argv);

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
