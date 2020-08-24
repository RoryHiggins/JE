#include "stdafx.h"

#if !defined(JE_WINDOW_H)
#define JE_WINDOW_H

#include "core.h"
#include "client.h"
#include "rendering.h"
#include "lua_wrapper.h"


typedef struct {
	sfRenderWindow* window;
	sfRenderStates renderStates;
	sfTexture* spriteTexture;
	jeRenderQueue renderQueue;

	time_t lastSecond;
	unsigned framesThisSecond;
	unsigned framesLastSecond;

	bool closing;
} jeWindow;

jeWindow* jeWindow_get();
bool jeWindow_isOpen(jeWindow* window);
void jeWindow_destroy(jeWindow* window);
bool jeWindow_create(jeWindow* window);
void jeWindow_step(jeWindow* window);

#endif
