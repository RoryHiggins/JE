#pragma once

#if !defined(JE_PLATFORM_RENDERING_H)
#define JE_PLATFORM_RENDERING_H

#include <j25/core/api.h>
#include <j25/core/container.h>

#include <stdbool.h>
#include <stdint.h>

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

JE_API_PUBLIC uint32_t jePrimitiveType_getVertexCount(uint32_t primitiveType);


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
JE_API_PUBLIC const char* jeVertex_getDebugString(const struct jeVertex* vertex);
JE_API_PUBLIC const char* jeVertex_arrayGetDebugString(const struct jeVertex* vertices, uint32_t vertexCount);
JE_API_PUBLIC void jeVertex_createPointQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex pointVertices[JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT]);
JE_API_PUBLIC void jeVertex_createLineQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex lineVertices[JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT]);
JE_API_PUBLIC void jeVertex_createSpriteQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex spriteVertices[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT]);


struct jeVertexBuffer {
	struct jeArray vertices;
};
JE_API_PUBLIC bool jeVertexBuffer_create(struct jeVertexBuffer* vertexBuffer);
JE_API_PUBLIC void jeVertexBuffer_destroy(struct jeVertexBuffer* vertexBuffer);
JE_API_PUBLIC void jeVertexBuffer_reset(struct jeVertexBuffer* vertexBuffer);
JE_API_PUBLIC bool jeVertexBuffer_sort(struct jeVertexBuffer* vertexBuffer, uint32_t primitiveType);
JE_API_PUBLIC void jeVertexBuffer_pushPrimitive(struct jeVertexBuffer* vertexBuffer, const struct jeVertex* vertices, uint32_t primitiveType);


struct jeColorRGBA32 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};


struct jeImage {
	uint32_t width;
	uint32_t height;
	struct jeArray buffer;
};
JE_API_PUBLIC bool jeImage_create(struct jeImage* image, uint32_t width, uint32_t height, struct jeColorRGBA32 fillColor);
JE_API_PUBLIC bool jeImage_createFromFile(struct jeImage* image, const char* filename);
JE_API_PUBLIC void jeImage_destroy(struct jeImage* image);


JE_API_PUBLIC void jeRendering_runTests();

#endif
