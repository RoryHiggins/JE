#include <j25/core/common.h>
#include <j25/client/client.h>

#include <stdlib.h>

int main(int argc, char** argv) {
	bool ok = jeClient_run(argc, argv);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
