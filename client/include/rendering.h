#if !defined(JE_RENDERING_H)
#define JE_RENDERING_H

#include "common.h"
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
struct jeVertexArray {
	struct jeArray vertices;
};

JE_PUBLIC int jePrimitiveType_getVertexCount(int primitiveType);

JE_PUBLIC const char* jeVertex_toDebugString(const struct jeVertex* vertices);
JE_PUBLIC const char* jeVertex_arrayToDebugString(const struct jeVertex* vertices, int vertexCount);
JE_PUBLIC void jeVertex_createPointQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex pointVertices[JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT]);
JE_PUBLIC void jeVertex_createLineQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex lineVertices[JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT]);
JE_PUBLIC void jeVertex_createSpriteQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex spriteVertices[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT]);

JE_PUBLIC void jeVertexBuffer_destroy(struct jeVertexArray* vertexBuffer);
JE_PUBLIC jeBool jeVertexBuffer_create(struct jeVertexArray* vertexBuffer);
JE_PUBLIC void jeVertexBuffer_reset(struct jeVertexArray* vertexBuffer);
JE_PUBLIC jeBool jeVertexBuffer_sort(struct jeVertexArray* vertexBuffer, int primitiveType);
JE_PUBLIC void jeVertexBuffer_pushPrimitive(struct jeVertexArray* vertexBuffer, const struct jeVertex* vertices, int primitiveType);

JE_PUBLIC void jeRendering_runTests();

#endif
