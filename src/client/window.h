#if !defined(JE_WINDOW_H)
#define JE_WINDOW_H

#include "core.h"

#define JE_INPUT_LEFT 0
#define JE_INPUT_UP 1
#define JE_INPUT_RIGHT 2
#define JE_INPUT_DOWN 3
#define JE_INPUT_A 4
#define JE_INPUT_B 5
#define JE_INPUT_X 6
#define JE_INPUT_Y 7


typedef struct jeWindow jeWindow;

jeWindow* jeWindow_get();
bool jeWindow_isOpen(jeWindow* window);
void jeWindow_destroy(jeWindow* window);
bool jeWindow_create(jeWindow* window);
void jeWindow_step(jeWindow* window);
bool jeWindow_getInput(jeWindow* window, int inputId);
unsigned jeWindow_getFramesPerSecond(jeWindow* window);
void jeWindow_drawSprite(jeWindow* window, int z, float x1, float y1, float x2, float y2, float r, float g, float b, float a, float u1, float v1, float u2, float v2);

#endif
