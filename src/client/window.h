#if !defined(JE_WINDOW_H)
#define JE_WINDOW_H

#include "stdafx.h"
#include "image.h"

#define JE_INPUT_LEFT 0
#define JE_INPUT_UP 1
#define JE_INPUT_RIGHT 2
#define JE_INPUT_DOWN 3
#define JE_INPUT_A 4
#define JE_INPUT_B 5
#define JE_INPUT_X 6
#define JE_INPUT_Y 7
#define JE_INPUT_COUNT 8

#define JE_WINDOW_VERTEX_BUFFER_CAPACITY 16 * 1024


typedef struct jeWindow jeWindow;

jeWindow* jeWindow_create();
void jeWindow_step(jeWindow* window);
void jeWindow_destroy(jeWindow* window);

void jeWindow_drawSprite(jeWindow* window,
						 float z, float x1, float y1, float x2, float y2,
						 float r, float g, float b, float a,
						 float u1, float v1, float u2, float v2);

jeBool jeWindow_getIsOpen(jeWindow* window);
int jeWindow_getFramesPerSecond(jeWindow* window);
jeBool jeWindow_getInputState(jeWindow* window, int inputId);

#endif
