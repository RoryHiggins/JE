#include <j25/platform/image.h>

#if defined(JE_BUILD_HEADLESS)

#include <j25/core/container.h>
#include <j25/core/debug.h>

#include <stdbool.h>

bool jeImage_create(struct jeImage* image, uint32_t width, uint32_t height, struct jeColorRGBA32 fillColor) {
	JE_MAYBE_UNUSED(image);
	JE_MAYBE_UNUSED(width);
	JE_MAYBE_UNUSED(height);
	JE_MAYBE_UNUSED(fillColor);

	return false;
}
bool jeImage_createFromFile(struct jeImage* image, const char* filename) {
	JE_MAYBE_UNUSED(image);
	JE_MAYBE_UNUSED(filename);

	return false;
}
void jeImage_destroy(struct jeImage* image) {
	JE_MAYBE_UNUSED(image);
}

void jeImage_runTests() {
}

#endif
