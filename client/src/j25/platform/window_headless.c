#include <j25/platform/window.h>

#if defined(JE_BUILD_HEADLESS)

#include <j25/core/container.h>
#include <j25/core/debug.h>
#include <j25/platform/rendering.h>

#include <stdbool.h>

struct jeWindow {
	bool unused;
};

bool jeWindow_getIsOpen(const struct jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return false;
}
uint32_t jeWindow_getWidth(const struct jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return 0;
}
uint32_t jeWindow_getHeight(const struct jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return 0;
}
bool jeWindow_getIsValid(struct jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return true;
}
void jeWindow_resetPrimitives(struct jeWindow* window) {
	JE_MAYBE_UNUSED(window);
}
void jeWindow_pushPrimitive(struct jeWindow* window, const struct jeVertex* vertices, uint32_t primitiveType) {
	JE_MAYBE_UNUSED(window);
	JE_MAYBE_UNUSED(vertices);
	JE_MAYBE_UNUSED(primitiveType);
}
void jeWindow_show(struct jeWindow* window) {
	JE_MAYBE_UNUSED(window);
}
bool jeWindow_step(struct jeWindow* window) {
	JE_MAYBE_UNUSED(window);
	return false;
}
void jeWindow_destroy(struct jeWindow* window) {
	JE_MAYBE_UNUSED(window);
}
struct jeWindow* jeWindow_create(bool startVisible, const char* optSpritesFilename) {
	JE_MAYBE_UNUSED(startVisible);
	JE_MAYBE_UNUSED(optSpritesFilename);

	return (struct jeWindow*)NULL;
}
bool jeWindow_getInput(const struct jeWindow* window, uint32_t inputId) {
	JE_MAYBE_UNUSED(window);
	JE_MAYBE_UNUSED(inputId);

	return false;
}
uint32_t jeWindow_getFps(const struct jeWindow* window) {
	JE_MAYBE_UNUSED(window);

	return 0;
}

void jeWindow_runTests() {
}

#endif
