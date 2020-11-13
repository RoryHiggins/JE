#if !defined(JE_WINDOW_H)
#define JE_WINDOW_H

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

struct jeWindow;
void jeWindow_destroy(struct jeWindow* window);
struct jeWindow* jeWindow_create(bool startVisible, const char* optSpritesFilename);
void jeWindow_show(struct jeWindow* window);
bool jeWindow_step(struct jeWindow* window);
void jeWindow_resetPrimitives(struct jeWindow* window);
void jeWindow_pushPrimitive(struct jeWindow* window, const struct jeVertex* vertices, int primitiveType);
bool jeWindow_getIsOpen(const struct jeWindow* window);
int jeWindow_getFps(const struct jeWindow* window);
bool jeWindow_getInput(const struct jeWindow* window, int inputId);
void jeWindowRunTests();

#endif
