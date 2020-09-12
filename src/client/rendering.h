#if !defined(JE_RENDERABLE_H)
#define JE_RENDERABLE_H

#include "stdafx.h"
#include "image.h"

#define JE_PRIMITIVE_TYPE_UNKNOWN 0
#define JE_PRIMITIVE_TYPE_POINTS 1
#define JE_PRIMITIVE_TYPE_LINES 2
#define JE_PRIMITIVE_TYPE_TRIANGLES 3
#define JE_PRIMITIVE_TYPE_SPRITES 4

#define JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT 1
#define JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT 2
#define JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT 3
#define JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT (JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT * 2)

#define JE_RENDERABLE_VERTEX_COUNT JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT


typedef int jePrimitiveType;
int jePrimitiveType_getVertexCount(jePrimitiveType primitiveType);

typedef struct jeVertex jeVertex;
struct jeVertex {
	float x;
	float y;
	float z;
	float w;

	float r;
	float g;
	float b;
	float a;

	float u;
	float v;
};
const char* jeVertex_toDebugString(const jeVertex* vertex);

typedef struct jeRenderable jeRenderable;
struct jeRenderable {
	jeVertex vertex[JE_RENDERABLE_VERTEX_COUNT];  /*number actually used depends on primitiveType*/

	float z;  /*primary sort key, to ensure translucent objects blend correctly*/
	jePrimitiveType primitiveType;  /*secondary sort key, to minimize number of opengl draw calls*/
};
const char* jeRenderable_toDebugString(const jeRenderable* renderable);
int jeRenderable_less(const void* aRaw, const void* bRaw);

/*Z-sorted queue of renderable objects*/
typedef struct jeRenderQueue jeRenderQueue;
struct jeRenderQueue {
	jeRenderable* renderables;
	int capacity;
	int count;
};
void jeRenderQueue_destroy(jeRenderQueue* renderQueue);
void jeRenderQueue_create(jeRenderQueue* renderQueue);
void jeRenderQueue_setCapacity(jeRenderQueue* renderQueue, int capacity);
void jeRenderQueue_insert(jeRenderQueue* renderQueue, jeRenderable* renderable);
void jeRenderQueue_sort(jeRenderQueue* renderQueue);

#endif
