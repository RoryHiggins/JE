#pragma once
#include "stdafx.h"
#include "logging.h"
#include "rendering.h"

#define JE_WINDOW_WIDTH 640
#define JE_WINDOW_HEIGHT 480
#define JE_WINDOW_SCALE 4
#define JE_WINDOW_FRAME_RATE 30
#define JE_WINDOW_SPRITES "data/sprites.png"

typedef struct {
	sfRenderWindow* window;
	sfRenderStates renderStates;
	sfTexture* spriteTexture;
	// TODO remove in favor of a render queue
	sfSprite* sprite;
	jeRenderQueue renderQueue;

	time_t lastSecond;
	unsigned framesThisSecond;
	unsigned framesLastSecond;

	bool closing;
} jeWindow;

jeWindow* jeWindow_get() {
	static jeWindow window;
	return &window;
}
bool jeWindow_isOpen(jeWindow* window) {
	return (window->window != NULL) && !window->closing;
}
void jeWindow_destroy(jeWindow* window) {
	JE_LOG("jeWindow_destroy()");

	if (window->sprite != NULL) {
		sfSprite_destroy(window->sprite);
		window->sprite = NULL;
	}

	if (window->spriteTexture != NULL) {
		sfTexture_destroy(window->spriteTexture);
		window->spriteTexture = NULL;
	}

	if (window->window != NULL) {
		sfRenderWindow_destroy(window->window);
		window->window = NULL;
	}
}
bool jeWindow_create(jeWindow* window) {
	bool success = false;
	sfVideoMode videoMode;

	JE_LOG("jeWindow_create()");

	videoMode.width = JE_WINDOW_WIDTH;
	videoMode.height = JE_WINDOW_HEIGHT;
	videoMode.bitsPerPixel = 32;

	window->window = sfRenderWindow_create(videoMode, "", sfTitlebar | sfClose, NULL);
	if (window->window == NULL) {
		JE_ERR("jeWindow_create(): sfRenderWindow_create() failed");
		goto cleanup;
	}

	sfRenderWindow_setFramerateLimit(window->window, JE_WINDOW_FRAME_RATE);
	sfRenderWindow_clear(window->window, sfWhite);

	window->renderStates.blendMode = sfBlendAlpha;
	window->renderStates.transform = sfTransform_Identity;
	window->renderStates.texture = NULL;
	window->renderStates.shader = NULL;
	sfTransform_scale(&window->renderStates.transform, (float)JE_WINDOW_SCALE, (float)JE_WINDOW_SCALE);

	window->spriteTexture = sfTexture_createFromFile(JE_WINDOW_SPRITES, /*area*/ NULL);
	if (window->spriteTexture == NULL) {
		JE_ERR("jeWindow_create(): sfTexture_createFromFile() failed");
		goto cleanup;
	}
	window->sprite = sfSprite_create();
	if (window->sprite == NULL) {
		JE_ERR("jeWindow_create(): sfSprite_create() failed");
		goto cleanup;
	}
	sfSprite_setTexture(window->sprite, window->spriteTexture, /* resetRect */ true);
	window->renderStates.texture = window->spriteTexture;

	if (!jeRenderQueue_create(&window->renderQueue)) {
		goto cleanup;
	}

	time(&window->lastSecond);
	window->framesThisSecond = 0;
	window->framesLastSecond = 0;

	window->closing = false;

	success = true;
	cleanup: {
		if (!success) {
			jeWindow_destroy(window);
		}
	}

	return success;
}
void jeWindow_step(jeWindow* window) {
	sfEvent event;
	time_t timeNow;

	while (sfRenderWindow_pollEvent(window->window, &event)) {
		if (event.type == sfEvtClosed) {
			window->closing = true;
		}
	}

	if (sfKeyboard_isKeyPressed(sfKeyEscape)) {
		window->closing = true;
	}

	jeRenderQueue_draw(&window->renderQueue, window->window, &window->renderStates);

	sfRenderWindow_display(window->window);
	sfRenderWindow_clear(window->window, sfWhite);

	window->framesThisSecond++;
	time(&timeNow);
	if (difftime(timeNow, window->lastSecond) >= 1.0) {
		window->lastSecond = timeNow;
		window->framesLastSecond = window->framesThisSecond;
		window->framesThisSecond = 0;
	}
}