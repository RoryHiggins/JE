#include "precompiled.h"
#include "window.h"
#include "core.h"

#define JE_WINDOW_WIDTH 640
#define JE_WINDOW_HEIGHT 480
#define JE_WINDOW_SCALE 4
#define JE_WINDOW_FRAME_RATE 30
#define JE_WINDOW_SPRITES "data/sprites.png"


typedef struct jeRenderVertexArray jeRenderVertexArray;
typedef struct jeRenderQueue jeRenderQueue;

struct jeRenderVertexArray {
	int z;
	sfVertexArray* vertexArray;
};

struct jeRenderQueue {
	jeRenderVertexArray* vertexArrays;

	int capacity;
	int count;
};

struct jeWindow {
	sfRenderWindow* window;
	sfRenderStates renderStates;
	sfTexture* spriteTexture;
	jeRenderQueue renderQueue;

	time_t lastSecond;
	unsigned framesThisSecond;
	unsigned framesLastSecond;

	bool closing;
};

int jeVertexArray_compare(const void* a, const void* b) {
	return ((const jeRenderVertexArray*)a)->z - ((const jeRenderVertexArray*)b)->z;
}
bool jeVertexArray_create(jeRenderVertexArray* vertexArray, int z) {
	vertexArray->z = z;
	vertexArray->vertexArray = sfVertexArray_create();
	sfVertexArray_setPrimitiveType(vertexArray->vertexArray, sfTriangles);

	return true;
}
void jeVertexArray_destroy(jeRenderVertexArray* vertexArray) {
	if (vertexArray->vertexArray != NULL) {
		sfVertexArray_destroy(vertexArray->vertexArray);
		vertexArray->vertexArray = NULL;
	}
}

jeRenderVertexArray jeRenderQueue_getArray(jeRenderQueue* renderQueue, int z) {
	jeRenderVertexArray vertexArray = {0};
	jeRenderVertexArray* vertexArrayPtr = NULL;
	int newCapacity = 0;

	vertexArray.z = z;
	vertexArrayPtr = (jeRenderVertexArray*)bsearch(&vertexArray, renderQueue->vertexArrays, renderQueue->count, sizeof(jeRenderVertexArray), &jeVertexArray_compare);

	if (vertexArrayPtr != NULL) {
		vertexArray = *vertexArrayPtr;
		goto cleanup;
	}

	if (renderQueue->count >= renderQueue->capacity) {
		newCapacity = (renderQueue->capacity * 2) + 1;
		renderQueue->vertexArrays = (jeRenderVertexArray*)realloc((void*)renderQueue->vertexArrays, sizeof(jeRenderVertexArray) * newCapacity);
		renderQueue->capacity = newCapacity;
	}

	jeVertexArray_create(&renderQueue->vertexArrays[renderQueue->count], z);
	vertexArray = renderQueue->vertexArrays[renderQueue->count];
	renderQueue->count++;
	qsort((void*)renderQueue->vertexArrays, renderQueue->count, sizeof(jeRenderVertexArray), &jeVertexArray_compare);

	cleanup: {
	}

	return vertexArray;
}
void jeRenderQueue_queueSprite(jeRenderQueue* renderQueue, int z, float x1, float y1, float x2, float y2, float r, float g, float b, float a, float u1, float v1, float u2, float v2) {
	jeRenderVertexArray vertexArray = jeRenderQueue_getArray(renderQueue, z);
	size_t vertexIndex = sfVertexArray_getVertexCount(vertexArray.vertexArray);
	sfVertex* vertex = NULL;
	int i = 0;

	sfVertexArray_resize(vertexArray.vertexArray, vertexIndex + 6);

	/*
	Render sprite via two triangles.  Triangle vertex indices are clockwise:

		0  1
		2  -

		-  4
		3  5
	*/

	vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 0);
	vertex->position.x = x1;
	vertex->position.y = y1;
	vertex->texCoords.x = u1;
	vertex->texCoords.y = v1;

	vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 1);
	vertex->position.x = x2;
	vertex->position.y = y1;
	vertex->texCoords.x = u2;
	vertex->texCoords.y = v1;

	vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 2);
	vertex->position.x = x1;
	vertex->position.y = y2;
	vertex->texCoords.x = u1;
	vertex->texCoords.y = v2;

	vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 3);
	vertex->position.x = x1;
	vertex->position.y = y2;
	vertex->texCoords.x = u1;
	vertex->texCoords.y = v2;

	vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 4);
	vertex->position.x = x2;
	vertex->position.y = y1;
	vertex->texCoords.x = u2;
	vertex->texCoords.y = v1;

	vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + 5);
	vertex->position.x = x2;
	vertex->position.y = y2;
	vertex->texCoords.x = u2;
	vertex->texCoords.y = v2;

	for (i = 0; i < 6; i++) {
		vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + i);
		vertex->color.r = r;
		vertex->color.g = g;
		vertex->color.b = b;
		vertex->color.a = a;
	}
}
void jeRenderQueue_draw(jeRenderQueue* renderQueue, sfRenderWindow *renderWindow, const sfRenderStates *renderStates) {
	int i = 0;

	/*draw layers back-to-front (painters algorithm)*/
	for (i = renderQueue->count - 1; i >= 0; i--) {
		sfRenderWindow_drawVertexArray(renderWindow, renderQueue->vertexArrays[i].vertexArray, renderStates);
		sfVertexArray_clear(renderQueue->vertexArrays[i].vertexArray);
	}
}
bool jeRenderQueue_create(jeRenderQueue* renderQueue) {
	static const int startCapacity = 16;

	renderQueue->vertexArrays = (jeRenderVertexArray*)malloc(sizeof(jeRenderVertexArray) * startCapacity);
	renderQueue->count = 0;
	renderQueue->capacity = startCapacity;

	return true;
}
void jeRenderQueue_destroy(jeRenderQueue* renderQueue) {
	int i = 0;

	if (renderQueue->vertexArrays) {
		for (i = 0; i < renderQueue->count; i++) {
			jeVertexArray_destroy(&renderQueue->vertexArrays[i]);
		}
		free((void*)renderQueue->vertexArrays);
		renderQueue->vertexArrays = NULL;

		renderQueue->count = 0;
		renderQueue->capacity = 0;
	}
}

jeWindow* jeWindow_get() {
	static jeWindow window;
	return &window;
}
bool jeWindow_isOpen(jeWindow* window) {
	return (window->window != NULL) && (window->closing == false);
}
void jeWindow_destroy(jeWindow* window) {
	JE_LOG("jeWindow_destroy()");

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
	sfVideoMode videoMode = {0};

	JE_LOG("jeWindow_create()");

	videoMode.width = JE_WINDOW_WIDTH;
	videoMode.height = JE_WINDOW_HEIGHT;
	videoMode.bitsPerPixel = 32;

	memset((void*)window, 0, sizeof(*window));

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
	window->renderStates.texture = window->spriteTexture;

	if (jeRenderQueue_create(&window->renderQueue) == false) {
		goto cleanup;
	}

	time(&window->lastSecond);
	window->framesThisSecond = 0;
	window->framesLastSecond = 0;

	window->closing = false;

	success = true;
	cleanup: {
	}

	return success;
}
void jeWindow_step(jeWindow* window) {
	sfEvent event;
	time_t timeNow = 0;

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
bool jeWindow_getInput(jeWindow* window, int inputId) {
	switch (inputId) {
		case JE_INPUT_LEFT: {
			return sfKeyboard_isKeyPressed(sfKeyLeft) || sfKeyboard_isKeyPressed(sfKeyA);
		}
		case JE_INPUT_RIGHT: {
			return sfKeyboard_isKeyPressed(sfKeyRight) || sfKeyboard_isKeyPressed(sfKeyD);
		}
		case JE_INPUT_UP: {
			return sfKeyboard_isKeyPressed(sfKeyUp) || sfKeyboard_isKeyPressed(sfKeyW);
		}
		case JE_INPUT_DOWN: {
			return sfKeyboard_isKeyPressed(sfKeyDown) || sfKeyboard_isKeyPressed(sfKeyS);
		}
		case JE_INPUT_A: {
			return sfKeyboard_isKeyPressed(sfKeyEnter) || sfKeyboard_isKeyPressed(sfKeyZ);
		}
		case JE_INPUT_B: {
			return sfKeyboard_isKeyPressed(sfKeyBackspace) || sfKeyboard_isKeyPressed(sfKeyX);
		}
		case JE_INPUT_X: {
			return sfKeyboard_isKeyPressed(sfKeyC);
		}
		case JE_INPUT_Y: {
			return sfKeyboard_isKeyPressed(sfKeyC);
		}
	}

	JE_MAYBE_UNUSED(window);

	return false;
}
unsigned jeWindow_getFramesPerSecond(jeWindow* window) {
	return window->framesLastSecond;
}
void jeWindow_drawSprite(jeWindow* window, int z, float x1, float y1, float x2, float y2, float r, float g, float b, float a, float u1, float v1, float u2, float v2) {
	jeRenderQueue_queueSprite(&window->renderQueue, z, x1, y1, x2, y2, r, g, b, a, u1, v1, u2, v2);
}

