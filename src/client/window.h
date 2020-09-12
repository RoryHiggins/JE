#if !defined(JE_WINDOW_H)
#define JE_WINDOW_H

#include "stdafx.h"
#include "renderable.h"

#define JE_PRIMITIVE_TYPE_UNKNOWN 0
#define JE_PRIMITIVE_TYPE_POINTS 1
#define JE_PRIMITIVE_TYPE_LINES 2
#define JE_PRIMITIVE_TYPE_TRIANGLES 3
#define JE_PRIMITIVE_TYPE_SPRITES 4

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
void jeWindow_step(jeWindow* window);
void jeWindow_destroy(jeWindow* window);

void jeWindow_drawRenderable(jeWindow* window, jeRenderable* renderable);

jeBool jeWindow_getIsOpen(jeWindow* window);
int jeWindow_getFPS(jeWindow* window);
jeBool jeWindow_getInput(jeWindow* window, int inputId);

#endif
