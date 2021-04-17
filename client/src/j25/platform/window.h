#pragma once

#if !defined(JE_PLATFORM_WINDOW_H)
#define JE_PLATFORM_WINDOW_H

#include <j25/core/common.h>

#include <stdbool.h>
#include <stdint.h>

#define JE_INPUT_LEFT 0
#define JE_INPUT_UP 1
#define JE_INPUT_RIGHT 2
#define JE_INPUT_DOWN 3
#define JE_INPUT_A 4
#define JE_INPUT_B 5
#define JE_INPUT_X 6
#define JE_INPUT_Y 7
#define JE_INPUT_COUNT 8

#define JE_WINDOW_MIN_WIDTH 160
#define JE_WINDOW_MIN_HEIGHT 120

struct jeVertex;
struct jeWindow;

JE_API_PUBLIC void jeWindow_destroy(struct jeWindow* window);
JE_API_PUBLIC struct jeWindow* jeWindow_create(bool startVisible, const char* optSpritesFilename);
JE_API_PUBLIC void jeWindow_show(struct jeWindow* window);
JE_API_PUBLIC bool jeWindow_step(struct jeWindow* window);
JE_API_PUBLIC void jeWindow_resetPrimitives(struct jeWindow* window);
JE_API_PUBLIC void
jeWindow_pushPrimitive(struct jeWindow* window, const struct jeVertex* vertices, uint32_t primitiveType);
JE_API_PUBLIC bool jeWindow_getIsOpen(const struct jeWindow* window);
JE_API_PUBLIC uint32_t jeWindow_getFps(const struct jeWindow* window);
JE_API_PUBLIC bool jeWindow_getInput(const struct jeWindow* window, uint32_t inputId);
JE_API_PUBLIC uint32_t jeWindow_getWidth(const struct jeWindow* window);
JE_API_PUBLIC uint32_t jeWindow_getHeight(const struct jeWindow* window);
JE_API_PUBLIC bool jeWindow_getIsValid(struct jeWindow* window);

JE_API_PUBLIC void jeWindow_runTests();

#endif
