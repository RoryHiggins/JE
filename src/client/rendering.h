#pragma once
#include "stdafx.h"
#include "logging.h"

typedef struct {
	int z;
	sfVertexArray* vertexArray;
} jeVertexArray;
int jeVertexArray_compare(const void* a, const void* b) {
	return ((const jeVertexArray*)a)->z - ((const jeVertexArray*)b)->z;
}
bool jeVertexArray_create(jeVertexArray* vertexArray, int z) {
	vertexArray->z = z;
	vertexArray->vertexArray = sfVertexArray_create();
	sfVertexArray_setPrimitiveType(vertexArray->vertexArray, sfTriangles);

	return true;
}
void jeVertexArray_destroy(jeVertexArray* vertexArray) {
	if (vertexArray->vertexArray) {
		sfVertexArray_destroy(vertexArray->vertexArray);
		vertexArray->vertexArray = NULL;
	}
}

typedef struct {
	jeVertexArray* vertexArrays;

	int capacity;
	int count;
} jeRenderQueue;
jeVertexArray jeRenderQueue_get(jeRenderQueue* renderQueue, int z) {
	jeVertexArray vertexArray;
	vertexArray.z = z;
	jeVertexArray* vertexArrayPtr = (jeVertexArray*)bsearch(&vertexArray, renderQueue->vertexArrays, renderQueue->count, sizeof(jeVertexArray), &jeVertexArray_compare);

	if (vertexArrayPtr != NULL) {
		vertexArray = *vertexArrayPtr;
		goto cleanup;
	}

	if (renderQueue->count >= renderQueue->capacity) {
		int newCapacity = (renderQueue->capacity * 2) + 1;
		renderQueue->vertexArrays = (jeVertexArray*)realloc((void*)renderQueue->vertexArrays, sizeof(jeVertexArray) * newCapacity);
		renderQueue->capacity = newCapacity;
	}

	jeVertexArray_create(&renderQueue->vertexArrays[renderQueue->count], z);
	vertexArray = renderQueue->vertexArrays[renderQueue->count];
	renderQueue->count++;
	qsort((void*)renderQueue->vertexArrays, renderQueue->count, sizeof(jeVertexArray), &jeVertexArray_compare);

	cleanup: {
	}

	return vertexArray;
}
void jeRenderQueue_queueSprite(jeRenderQueue* renderQueue, int z, float x1, float y1, float x2, float y2, float r, float g, float b, float a, float u1, float v1, float u2, float v2) {
	jeVertexArray vertexArray;
	size_t vertexIndex;
	sfVertex* vertex;

	vertexArray = jeRenderQueue_get(renderQueue, z);
	vertexIndex = sfVertexArray_getVertexCount(vertexArray.vertexArray);
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

	for (int i = 0; i < 6; i++) {
		vertex = sfVertexArray_getVertex(vertexArray.vertexArray, vertexIndex + i);
		vertex->color.r = r;
		vertex->color.g = g;
		vertex->color.b = b;
		vertex->color.a = a;
	}
}
void jeRenderQueue_draw(jeRenderQueue* renderQueue, sfRenderWindow *renderWindow, const sfRenderStates *renderStates) {
	// draw layers back-to-front (painters algorithm)
	for (int i = renderQueue->count - 1; i >= 0; i--) {
		sfRenderWindow_drawVertexArray(renderWindow, renderQueue->vertexArrays[i].vertexArray, renderStates);
		sfVertexArray_resize(renderQueue->vertexArrays[i].vertexArray, 0);
	}
}
bool jeRenderQueue_create(jeRenderQueue* renderQueue) {
	const int startCapacity = 16;
	renderQueue->vertexArrays = (jeVertexArray*)malloc(sizeof(jeVertexArray) * startCapacity);
	renderQueue->count = 0;
	renderQueue->capacity = startCapacity;

	return true;
}
void jeRenderQueue_destroy(jeRenderQueue* renderQueue) {
	if (renderQueue->vertexArrays) {
		for (int i = 0; i < renderQueue->count; i++) {
			jeVertexArray_destroy(&renderQueue->vertexArrays[i]);
		}
		free((void*)renderQueue->vertexArrays);
		renderQueue->vertexArrays = NULL;

		renderQueue->count = 0;
		renderQueue->capacity = 0;
	}
}