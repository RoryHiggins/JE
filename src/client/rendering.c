#include "stdafx.h"
#include "rendering.h"
#include "core.h"



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