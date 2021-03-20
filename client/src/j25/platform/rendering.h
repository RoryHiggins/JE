#pragma once

#if !defined(JE_PLATFORM_RENDERING_H)
#define JE_PLATFORM_RENDERING_H

#include <j25/core/api.h>
#include <j25/core/container.h>

#include <stdbool.h>

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

JE_PUBLIC int jePrimitiveType_getVertexCount(int primitiveType);


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
JE_PUBLIC const char* jeVertex_toDebugString(const struct jeVertex* vertex);
JE_PUBLIC const char* jeVertex_arrayToDebugString(const struct jeVertex* vertices, int vertexCount);
JE_PUBLIC void jeVertex_createPointQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex pointVertices[JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT]);
JE_PUBLIC void jeVertex_createLineQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex lineVertices[JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT]);
JE_PUBLIC void jeVertex_createSpriteQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex spriteVertices[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT]);


struct jeVertexBuffer {
	struct jeArray vertices;
};
JE_PUBLIC bool jeVertexBuffer_create(struct jeVertexBuffer* vertexBuffer);
JE_PUBLIC void jeVertexBuffer_destroy(struct jeVertexBuffer* vertexBuffer);
JE_PUBLIC void jeVertexBuffer_reset(struct jeVertexBuffer* vertexBuffer);
JE_PUBLIC bool jeVertexBuffer_sort(struct jeVertexBuffer* vertexBuffer, int primitiveType);
JE_PUBLIC void jeVertexBuffer_pushPrimitive(struct jeVertexBuffer* vertexBuffer, const struct jeVertex* vertices, int primitiveType);


struct jeColorRGBA32 {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};


struct jeImage {
	int width;
	int height;
	struct jeArray buffer;
};
JE_PUBLIC bool jeImage_create(struct jeImage* image, int width, int height, struct jeColorRGBA32 fillColor);
JE_PUBLIC bool jeImage_createFromFile(struct jeImage* image, const char* filename);
JE_PUBLIC void jeImage_destroy(struct jeImage* image);


JE_PUBLIC void jeRendering_runTests();

#endif
