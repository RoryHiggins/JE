#if !defined(JE_WINDOW_H)
#define JE_WINDOW_H

#include "stdafx.h"
#include "rendering.h"

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


typedef struct jeWindow jeWindow;
jeWindow* jeWindow_create();
void jeWindow_destroy(jeWindow* window);
void jeWindow_step(jeWindow* window);
void jeWindow_pushPrimitive(jeWindow* window, const jeVertex* vertices, jePrimitiveType primitiveType);
bool jeWindow_getIsOpen(jeWindow* window);
int jeWindow_getFps(jeWindow* window);
bool jeWindow_getInput(jeWindow* window, int inputId);

#endif
