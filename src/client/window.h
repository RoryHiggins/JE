#if !defined(JE_WINDOW_H)
#define JE_WINDOW_H

#include "stdafx.h"
#include "image.h"

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


/*TODO move Renderable and RenderQueue to their own file*/
typedef struct jeRenderable jeRenderable;
struct jeRenderable {
	float z;  /*primary sort key, to ensure translucent objects blend correctly*/
	int primitiveType;  /*secondary sort key, to minimize number of opengl draw calls*/

	float x1;
	float y1;
	float x2;
	float y2;
	float x3;
	float y3;

	float r;
	float g;
	float b;
	float a;

	float u1;
	float v1;
	float u2;
	float v2;
	float u3;
	float v3;
};

const char* jeRenderable_toDebugString(const jeRenderable* renderable);

typedef struct jeWindow jeWindow;

jeWindow* jeWindow_create();
void jeWindow_step(jeWindow* window);
void jeWindow_destroy(jeWindow* window);

void jeWindow_drawRenderable(jeWindow* window, jeRenderable renderable);

jeBool jeWindow_getIsOpen(jeWindow* window);
int jeWindow_getFramesPerSecond(jeWindow* window);
jeBool jeWindow_getInputState(jeWindow* window, int inputId);

#endif
