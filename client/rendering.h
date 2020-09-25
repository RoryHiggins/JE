#if !defined(JE_RENDERABLE_H)
#define JE_RENDERABLE_H

#include "stdafx.h"
#include "container.h"

#define JE_PRIMITIVE_TYPE_UNKNOWN 0
#define JE_PRIMITIVE_TYPE_POINTS 1
#define JE_PRIMITIVE_TYPE_LINES 2
#define JE_PRIMITIVE_TYPE_SPRITES 3
#define JE_PRIMITIVE_TYPE_TRIANGLES 4
#define JE_PRIMITIVE_TYPE_QUADS 5
#define JE_PRIMITIVE_TYPE_COUNT 5

#define JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT 1
#define JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT 2
#define JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT 2
#define JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT 3
#define JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT 6
#define JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT 6


typedef struct jeVertex jeVertex;
struct jeVertex {
	float x;
	float y;
	float z;
	float unused;

	float r;
	float g;
	float b;
	float a;

	float u;
	float v;
};

typedef int jePrimitiveType;
int jePrimitiveType_getVertexCount(jePrimitiveType primitiveType);

const char* jeVertex_toDebugString(const jeVertex* vertices);
const char* jeVertex_arrayToDebugString(const jeVertex* vertices, int vertexCount);
void jeVertex_createPointQuad(jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const jeVertex pointVertices[JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT]);
void jeVertex_createLineQuad(jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const jeVertex lineVertices[JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT]);
void jeVertex_createSpriteQuad(jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const jeVertex spriteVertices[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT]);

typedef struct jeVertexBuffer jeVertexBuffer;
struct jeVertexBuffer {
	jeBuffer vertices;
};
void jeVertexBuffer_destroy(jeVertexBuffer* vertexBuffer);
bool jeVertexBuffer_create(jeVertexBuffer* vertexBuffer);
void jeVertexBuffer_reset(jeVertexBuffer* vertexBuffer);
bool jeVertexBuffer_sort(jeVertexBuffer* vertexBuffer, jePrimitiveType primitiveType);
void jeVertexBuffer_pushPrimitive(jeVertexBuffer* vertexBuffer, const jeVertex* vertices, jePrimitiveType primitiveType);

void jeRenderingRunTests();

#endif
