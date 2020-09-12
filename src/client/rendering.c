#include "stdafx.h"
#include "rendering.h"
#include "debug.h"

int jePrimitiveType_getVertexCount(jePrimitiveType primitiveType) {
	int vertexCount = 0;

	switch (primitiveType) {
		case JE_PRIMITIVE_TYPE_POINTS: {
			vertexCount = JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT;
			break;
		}
		case JE_PRIMITIVE_TYPE_LINES: {
			vertexCount = JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT;
			break;
		}
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			vertexCount = JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT;
			break;
		}
		case JE_PRIMITIVE_TYPE_SPRITES: {
			vertexCount = JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT;
			break;
		}
	}

	return vertexCount;
}

const char* jeVertex_toDebugString(const jeVertex* vertex) {
	static char buffer[512];

	memset((void*)buffer, 0, sizeof(buffer));
	sprintf(
		buffer,
		"x=%.2f, y=%.2f, z=%.2f, r=%.2f, g=%.2f, b=%.2f, a=%.2f, u=%.2f, v=%.2f",
		vertex->x, vertex->y, vertex->z, vertex->r, vertex->g, vertex->b, vertex->a, vertex->u, vertex->v
	);

	return buffer;
}
const char* jeRenderable_toDebugString(const jeRenderable* renderable) {
	static char buffer[2048];

	int i = 0;
	int bufferSize = 0;

	memset((void*)buffer, 0, sizeof(buffer));

	bufferSize += sprintf(buffer + bufferSize, "z=%.2f, primitiveType=%d", renderable->z, renderable->primitiveType);

	for (i = 0; i < JE_RENDERABLE_VERTEX_COUNT; i++) {
		bufferSize += sprintf(buffer + bufferSize, ", vertex[%d]={%s}", i, jeVertex_toDebugString(&renderable->vertex[i]));
	}

	return buffer;
}
int jeRenderable_less(const void* aRaw, const void* bRaw) {
	const jeRenderable* a = (const jeRenderable*)aRaw;
	const jeRenderable* b = (const jeRenderable*)bRaw;

	if (a->z == b->z) {
		return a->primitiveType < b->primitiveType;
	}
	return (a->z < b->z);
}

void jeRenderQueue_destroy(jeRenderQueue* renderQueue) {
	renderQueue->count = 0;
	renderQueue->capacity = 0;

	if (renderQueue->renderables != NULL) {
		free(renderQueue->renderables);
		renderQueue->renderables = NULL;
	}
}
void jeRenderQueue_create(jeRenderQueue* renderQueue) {
	renderQueue->renderables = NULL;
	renderQueue->capacity = 0;
	renderQueue->count = 0;
}
void jeRenderQueue_setCapacity(jeRenderQueue* renderQueue, int capacity) {
	JE_DEBUG("jeRenderQueue_setCapacity(): newCapacity=%d, currentCapacity=%d", capacity, renderQueue->capacity);

	if (capacity == renderQueue->capacity) {
		goto finalize;
	}

	if (capacity == 0) {
		jeRenderQueue_destroy(renderQueue);
		goto finalize;
	}

	if (renderQueue->renderables == NULL) {
		renderQueue->renderables = (jeRenderable*)malloc(sizeof(jeRenderable) * capacity);
	} else {
		renderQueue->renderables = (jeRenderable*)realloc(renderQueue->renderables, sizeof(jeRenderable) * capacity);
	}

	renderQueue->capacity = capacity;

	if (renderQueue->count > capacity) {
		renderQueue->count = capacity;
	}

	finalize: {
	}
}
void jeRenderQueue_insert(jeRenderQueue* renderQueue, jeRenderable* renderable) {
	static const int startCapacity = 32;

	int newCapacity = 0;

	if (renderQueue->count >= renderQueue->capacity) {
		newCapacity = startCapacity;
		if (renderQueue->capacity >= startCapacity) {
			newCapacity = renderQueue->capacity * 4;
		}
		jeRenderQueue_setCapacity(renderQueue, newCapacity);
	}

	renderQueue->renderables[renderQueue->count] = *renderable;
	renderQueue->count++;
}
void jeRenderQueue_sort(jeRenderQueue* renderQueue) {
	qsort(renderQueue->renderables, renderQueue->count, sizeof(jeRenderable), jeRenderable_less);
}