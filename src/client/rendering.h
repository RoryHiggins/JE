#if !defined(JE_RENDERABLE_H)
#define JE_RENDERABLE_H

#include "stdafx.h"
#include "container.h"

#define JE_PRIMITIVE_TYPE_UNKNOWN 0
#define JE_PRIMITIVE_TYPE_POINTS 1
#define JE_PRIMITIVE_TYPE_LINES 2
#define JE_PRIMITIVE_TYPE_TRIANGLES 3
#define JE_PRIMITIVE_TYPE_SPRITES 4

#define JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT 1
#define JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT 2
#define JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT 3
#define JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT (JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT * 2)

#define JE_RENDERABLE_VERTEX_COUNT 2


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
const char* jeVertex_toDebugString(const jeVertex* vertices);

typedef struct jeTriangle jeTriangle;
struct jeTriangle {
	jeVertex vertices[3];
};
const char* jeTriangle_toDebugString(const jeTriangle* vertices);
int jeTriangle_less(const void* triangleARaw, const void* triangleBRaw);


typedef struct jeRenderable jeRenderable;
struct jeRenderable {
	jeVertex vertex[JE_RENDERABLE_VERTEX_COUNT];  /*number actually used depends on primitiveType*/
};
const char* jeRenderable_toDebugString(const jeRenderable* renderable);


typedef struct jeVertexBuffer jeVertexBuffer;
struct jeVertexBuffer {
	jeBuffer vertices;
};
void jeVertexBuffer_destroy(jeVertexBuffer* vertexBuffer);
bool jeVertexBuffer_create(jeVertexBuffer* vertexBuffer);
void jeVertexBuffer_reset(jeVertexBuffer* vertexBuffer);
bool jeVertexBuffer_push(jeVertexBuffer* vertexBuffer, const jeVertex* vertices, int count);
void jeVertexBuffer_pushRenderable(jeVertexBuffer* vertexBuffer, const jeRenderable* renderable, jePrimitiveType primitiveType);

#endif
