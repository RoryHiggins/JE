#include "stdafx.h"

#if !defined(JE_RENDERING_H)
#define JE_RENDERING_H

#include "core.h"


typedef struct {
	int z;
	sfVertexArray* vertexArray;
} jeRenderVertexArray;
typedef struct {
	jeRenderVertexArray* vertexArrays;

	int capacity;
	int count;
} jeRenderQueue;

void jeRenderQueue_queueSprite(jeRenderQueue* renderQueue, int z, float x1, float y1, float x2, float y2, float r, float g, float b, float a, float u1, float v1, float u2, float v2);
void jeRenderQueue_draw(jeRenderQueue* renderQueue, sfRenderWindow *renderWindow, const sfRenderStates *renderStates);
bool jeRenderQueue_create(jeRenderQueue* renderQueue);
void jeRenderQueue_destroy(jeRenderQueue* renderQueue);

#endif
