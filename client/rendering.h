#if !defined(JE_RENDERABLE_H)
#define JE_RENDERABLE_H

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

int jePrimitiveType_getVertexCount(int primitiveType);

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
const char* jeVertex_toDebugString(const struct jeVertex* vertices);
const char* jeVertex_arrayToDebugString(const struct jeVertex* vertices, int vertexCount);
void jeVertex_createPointQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex pointVertices[JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT]);
void jeVertex_createLineQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex lineVertices[JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT]);
void jeVertex_createSpriteQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex spriteVertices[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT]);

struct jeVertexArray {
	struct jeArray vertices;
};
void jeVertexBuffer_destroy(struct jeVertexArray* vertexBuffer);
bool jeVertexBuffer_create(struct jeVertexArray* vertexBuffer);
void jeVertexBuffer_reset(struct jeVertexArray* vertexBuffer);
bool jeVertexBuffer_sort(struct jeVertexArray* vertexBuffer, int primitiveType);
void jeVertexBuffer_pushPrimitive(struct jeVertexArray* vertexBuffer, const struct jeVertex* vertices, int primitiveType);

void jeRenderingRunTests();

#endif
