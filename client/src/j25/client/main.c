#include <j25/core/common.h>
#include <j25/client/client.h>

#include <stdlib.h>

int main(int argc, char** argv) {
	bool ok = true;
	struct jeClient client;

	ok = ok && jeClient_run(&client, argc, argv);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
