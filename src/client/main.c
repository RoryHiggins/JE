#include "core.h"

/*Unity build*/
#include "core.c"
#include "image.c"
#include "window.c"
#include "lua_wrapper.c"
#include "client.c"


int main(int argc, char** argv) {
	bool success = true;

	success = jeClient_run();

	JE_MAYBE_UNUSED(argc);
	JE_MAYBE_UNUSED(argv);

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
