#if !defined(JE_PLATFORM_RENDERING_H)
#define JE_PLATFORM_RENDERING_H

#include <j25/stdafx.h>
#include <j25/core/container.h>

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

struct jeColor {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};
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
struct jeVertexBuffer {
	struct jeArray vertices;
};
struct jeImage {
	int width;
	int height;
	struct jeArray buffer;
};

JE_PUBLIC struct jeColor jeColor_getNone();
JE_PUBLIC struct jeColor jeColor_getBlack();
JE_PUBLIC struct jeColor jeColor_getWhite();
JE_PUBLIC struct jeColor jeColor_getDarkGray();
JE_PUBLIC struct jeColor jeColor_getGray();
JE_PUBLIC struct jeColor jeColor_getLightGray();
JE_PUBLIC struct jeColor jeColor_getRed();
JE_PUBLIC struct jeColor jeColor_getGreen();
JE_PUBLIC struct jeColor jeColor_getBlue();

JE_PUBLIC int jePrimitiveType_getVertexCount(int primitiveType);

JE_PUBLIC const char* jeVertex_toDebugString(const struct jeVertex* vertices);
JE_PUBLIC const char* jeVertex_arrayToDebugString(const struct jeVertex* vertices, int vertexCount);
JE_PUBLIC void jeVertex_createPointQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex pointVertices[JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT]);
JE_PUBLIC void jeVertex_createLineQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex lineVertices[JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT]);
JE_PUBLIC void jeVertex_createSpriteQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex spriteVertices[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT]);

JE_PUBLIC void jeVertexBuffer_destroy(struct jeVertexBuffer* vertexBuffer);
JE_PUBLIC bool jeVertexBuffer_create(struct jeVertexBuffer* vertexBuffer);
JE_PUBLIC void jeVertexBuffer_reset(struct jeVertexBuffer* vertexBuffer);
JE_PUBLIC bool jeVertexBuffer_sort(struct jeVertexBuffer* vertexBuffer, int primitiveType);
JE_PUBLIC void jeVertexBuffer_pushPrimitive(struct jeVertexBuffer* vertexBuffer, const struct jeVertex* vertices, int primitiveType);

JE_PUBLIC void jeImage_destroy(struct jeImage* image);
JE_PUBLIC bool jeImage_create(struct jeImage* image, int width, int height, struct jeColor fillColor);
JE_PUBLIC bool jeImage_createFromFile(struct jeImage* image, const char* filename);

JE_PUBLIC void jeRendering_runTests();

#endif
