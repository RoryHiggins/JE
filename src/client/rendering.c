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
int jeRenderable_less(const void* renderableARaw, const void* renderableBRaw) {
	const jeRenderable* renderableA = (const jeRenderable*)renderableARaw;
	const jeRenderable* renderableB = (const jeRenderable*)renderableBRaw;

	if (renderableA->z == renderableB->z) {
		return renderableA->primitiveType < renderableB->primitiveType;
	}

	return (renderableA->z < renderableB->z);
}

void jeRenderQueue_destroy(jeRenderQueue* renderQueue) {
	jeBuffer_destroy(&renderQueue->renderables);
}
bool jeRenderQueue_create(jeRenderQueue* renderQueue) {
	return jeBuffer_create(&renderQueue->renderables, sizeof(jeRenderable));
}
void jeRenderQueue_setCount(jeRenderQueue* renderQueue, int count) {
	jeBuffer_setCount(&renderQueue->renderables, count);
}
jeRenderable* jeRenderQueue_get(jeRenderQueue* renderQueue, int i) {
	return (jeRenderable*)jeBuffer_get(&renderQueue->renderables, i);
}
void jeRenderQueue_push(jeRenderQueue* renderQueue, const jeRenderable* renderable) {
	jeBuffer_push(&renderQueue->renderables, (const void*)renderable);
}
void jeRenderQueue_sort(jeRenderQueue* renderQueue) {
	qsort(renderQueue->renderables.data, renderQueue->renderables.count, sizeof(jeRenderable), jeRenderable_less);
}
