#include <j25/platform/rendering.h>

#include <j25/core/debug.h>
#include <j25/core/container.h>

#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <png.h>

#define JE_VERTEX_DEBUG_STRING_BUFFER_SIZE 128
#define JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES 16
#define JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE (JE_VERTEX_DEBUG_STRING_BUFFER_SIZE * JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES)

#define JE_PRIMITIVE_TYPE_DEBUG_STRING_BUFFER_SIZE JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE


/* Sort key which preserves both depth and order*/
struct jePrimitiveSortKey {
	float z;
	int index;
};

const char* jePrimitiveType_getDebugString(const struct jeVertex* vertices, int primitiveType);
int jePrimitiveSortKey_less(const void* rawSortKeyA, const void* rawSortKeyB);


int jePrimitiveType_getVertexCount(int primitiveType) {
	JE_TRACE("primitiveType=%d", primitiveType);

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
		case JE_PRIMITIVE_TYPE_SPRITES: {
			vertexCount = JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT;
			break;
		}
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			vertexCount = JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT;
			break;
		}
		case JE_PRIMITIVE_TYPE_QUADS: {
			vertexCount = JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT;
			break;
		}
		default: {
			JE_ERROR("unknown primitiveType, primitiveType=%d", primitiveType);
			vertexCount = 0;
			break;
		}
	}

	return vertexCount;
}
const char* jePrimitiveType_getDebugString(const struct jeVertex* vertices, int primitiveType) {
	JE_TRACE("vertices=%p, primitiveType=%d", (void*)vertices, primitiveType);

	int primitiveVertexCount = jePrimitiveType_getVertexCount(primitiveType);
	return jeVertex_arraygetDebugString(vertices, primitiveVertexCount);
}
int jePrimitiveSortKey_less(const void* rawSortKeyA, const void* rawSortKeyB) {
	const struct jePrimitiveSortKey* sortKeyA = (const struct jePrimitiveSortKey*)rawSortKeyA;
	const struct jePrimitiveSortKey* sortKeyB = (const struct jePrimitiveSortKey*)rawSortKeyB;

	int result = 0;
	if (sortKeyA->z > sortKeyB->z) {
		result = -1;
	} else if (sortKeyA->z < sortKeyB->z) {
		result = 1;
	} else if (sortKeyA->index < sortKeyB->index) {
		result = -1;
	} else if (sortKeyA->index > sortKeyB->index) {
		result = 1;
	}

	return result;
}

const char* jeVertex_getDebugString(const struct jeVertex* vertex) {
	JE_TRACE("vertex=%p", (void*)vertex);

	static char buffer[JE_VERTEX_DEBUG_STRING_BUFFER_SIZE];
	memset((void*)buffer, 0, sizeof(buffer));
	snprintf(
		buffer,
		JE_VERTEX_DEBUG_STRING_BUFFER_SIZE,
		"x=%.2f, y=%.2f, z=%.2f, r=%.2f, g=%.2f, b=%.2f, a=%.2f, u=%.2f, v=%.2f",
		vertex->x, vertex->y, vertex->z, vertex->r, vertex->g, vertex->b, vertex->a, vertex->u, vertex->v
	);

	return buffer;
}
const char* jeVertex_arraygetDebugString(const struct jeVertex* vertices, int vertexCount) {
	JE_TRACE("vertices=%p, vertexCount=%d", (void*)vertices, vertexCount);

	static char buffer[JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE];
	memset((void*)buffer, 0, sizeof(buffer));

	int printedVertexCount = vertexCount;
	if (printedVertexCount > JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES) {
		printedVertexCount = JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES;
	}

	int bufferOffset = 0;
	for (int i = 0; i < printedVertexCount; i++) {
		if (i > 0) {
			bufferOffset += snprintf(
				buffer + bufferOffset,
				JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE - bufferOffset,
				", "
			);
		}

		bufferOffset += snprintf(
			buffer + bufferOffset,
			JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE - bufferOffset,
			"vertex[%d]={%s}",
			i,
			jeVertex_getDebugString(&vertices[i])
		);
	}

	if (printedVertexCount < vertexCount) {
		bufferOffset += snprintf(
			buffer + bufferOffset,
			JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE - bufferOffset,
			", ..."
		);
	}

	return buffer;
}
void jeVertex_createPointQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex pointVertices[JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT]) {
	/* Render point as two triangles represented by 6 vertices.
	 *
	 * For visualization below, source indices = A..B, dest indices = 0..5,
	 * offset source indices = A+..B+ (x+=1 or y+=1 depending on line slope, see below)
	 *
	 *  0    2,3
	 *  x-----x+
	 *  | \   |
	 *  |   \ |
	 *  y+---xy+
	 * 1,4    5
	 *
	 */

	JE_TRACE("quadVertices=%p, pointVertices=%p", (void*)quadVertices, (void*)pointVertices);

	for (int i = 0; i < JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT; i++) {
		quadVertices[i] = *pointVertices;
	}

	/*Give the triangles actual width, as OpenGL won't render degenerate triangles*/
	static const float pointWidth = 1.0f;
	quadVertices[2].x += pointWidth;
	quadVertices[3].x += pointWidth;
	quadVertices[5].x += pointWidth;

	quadVertices[1].y += pointWidth;
	quadVertices[4].y += pointWidth;
	quadVertices[5].y += pointWidth;
}
void jeVertex_createLineQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex lineVertices[JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT]) {
	/* Render line as two triangles represented by 6 vertices.
	 *
	 * For visualization below, source indices = A..B, dest indices = 0..5,
	 * offset source indices = A+..B+ (x+=1 or y+=1 depending on line slope, see below)
	 *
	 * Vertical/mostly vertical line A to B:
	 * 0 A---A+ 1,4
	 *    \ / \
	 * 2,3 B---B+ 5
	 *
	 * Horizontal/mostly horizontal line A to B:
	 *  0    2,3
	 *  A-----B
	 *  | \   |
	 *  |   \ |
	 *  A+----B+
	 * 1,4    5
	 *
	 * NOTE: vertices are not necessarily clockwise, thus not compatible with back-face culling.
	 */

	JE_TRACE("quadVertices=%p, lineVertices=%p", (void*)quadVertices, (void*)lineVertices);

	for (int i = 0; i < JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT; i++) {
		quadVertices[i] = lineVertices[0];
	}

	quadVertices[2] = lineVertices[1];
	quadVertices[3] = lineVertices[1];
	quadVertices[5] = lineVertices[1];

	/*Give the triangles actual width, as OpenGL won't render degenerate triangles*/
	static const float lineWidth = 1.0f;
	float lengthX = fabs(lineVertices[1].x - lineVertices[0].x);
	float lengthY = fabs(lineVertices[1].y - lineVertices[0].y);
	float isHorizontalLine = lineWidth * (float)(lengthX > lengthY);
	float isVerticalLine = lineWidth * (float)(lengthX <= lengthY);
	quadVertices[1].x += isVerticalLine;
	quadVertices[1].y += isHorizontalLine;
	quadVertices[4].x += isVerticalLine;
	quadVertices[4].y += isHorizontalLine;
	quadVertices[5].x += isVerticalLine;
	quadVertices[5].y += isHorizontalLine;
}
void jeVertex_createSpriteQuad(struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const struct jeVertex spriteVertices[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT]) {
	/* Render sprite as two triangles represented by 6 clockwise vertices */

	JE_TRACE("quadVertices=%p, spriteVertices=%p", (void*)quadVertices, (void*)spriteVertices);

	for (int i = 0; i < JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT; i++) {
		quadVertices[i] = spriteVertices[0];
	}
	quadVertices[1].x = spriteVertices[1].x;
	quadVertices[1].u = spriteVertices[1].u;

	quadVertices[2].y = spriteVertices[1].y;
	quadVertices[2].v = spriteVertices[1].v;

	quadVertices[3].y = spriteVertices[1].y;
	quadVertices[3].v = spriteVertices[1].v;

	quadVertices[4].x = spriteVertices[1].x;
	quadVertices[4].u = spriteVertices[1].u;

	quadVertices[5].x = spriteVertices[1].x;
	quadVertices[5].y = spriteVertices[1].y;
	quadVertices[5].u = spriteVertices[1].u;
	quadVertices[5].v = spriteVertices[1].v;
}

bool jeVertexBuffer_create(struct jeVertexBuffer* vertexBuffer) {
	JE_TRACE("vertexBuffer=%p", (void*)vertexBuffer);

	return jeArray_create(&vertexBuffer->vertices, sizeof(struct jeVertex));
}
void jeVertexBuffer_destroy(struct jeVertexBuffer* vertexBuffer) {
	JE_TRACE("vertexBuffer=%p", (void*)vertexBuffer);

	jeArray_destroy(&vertexBuffer->vertices);
}
void jeVertexBuffer_reset(struct jeVertexBuffer* vertexBuffer) {
	jeArray_setCount(&vertexBuffer->vertices, 0);
}
bool jeVertexBuffer_sort(struct jeVertexBuffer* vertexBuffer, int primitiveType) {
	int primitiveVertexCount = jePrimitiveType_getVertexCount(primitiveType);
	int vertexCount = vertexBuffer->vertices.count;
	int primitiveCount = vertexCount / primitiveVertexCount;
	struct jeVertex *vertices = (struct jeVertex*)vertexBuffer->vertices.data;

	JE_TRACE("vertexBuffer=%p, vertexCount=%d, primitiveCount=%d", (void*)vertexBuffer, vertexCount, primitiveCount);

	bool ok = true;

	struct jeArray unsortedPrimitivesBuffer;
	ok = ok && jeArray_create(&unsortedPrimitivesBuffer, sizeof(struct jeVertex));
	ok = ok && jeArray_setCount(&unsortedPrimitivesBuffer, vertexCount);
	const struct jeVertex *unsortedVertices = (const struct jeVertex*)unsortedPrimitivesBuffer.data;
	if (vertices == NULL)
	{
		JE_TRACE("!");
	}
	memcpy((void*)unsortedVertices, (const void*)vertices, sizeof(struct jeVertex) * vertexCount);

	struct jeArray sortKeysBuffer;
	ok = ok && jeArray_create(&sortKeysBuffer, sizeof(struct jePrimitiveSortKey));
	ok = ok && jeArray_setCount(&sortKeysBuffer, primitiveCount);
	struct jePrimitiveSortKey *sortKeys = (struct jePrimitiveSortKey*)sortKeysBuffer.data;

	if (ok) {
		/*Generate sort keys array from vertices array*/
		for (int i = 0; i < primitiveCount; i++) {
			int primitiveVertexIndex = primitiveVertexCount * i;
			sortKeys[i].z = vertices[primitiveVertexIndex].z;
			sortKeys[i].index = primitiveVertexIndex;
		}

		/*Sort the sort keys array.  Uses both index and depth in order for sort to be stable*/
		qsort(sortKeys, primitiveCount, sizeof(struct jePrimitiveSortKey), jePrimitiveSortKey_less);

		/*Sort vertices using sort key order*/
		for (int i = 0; i < primitiveCount; i++) {
			int srcPrimitiveVertexIndex = sortKeys[i].index;
			int destPrimitiveVertexIndex = primitiveVertexCount * i;

			if ((srcPrimitiveVertexIndex + primitiveVertexCount - 1) > vertexCount) {
				JE_ERROR("src primitive starting index out of bounds, vertexBuffer=%p, index=%d, vertexCount=%d", (void*)vertexBuffer, srcPrimitiveVertexIndex, vertexCount);
				continue;
			}

			if ((destPrimitiveVertexIndex + primitiveVertexCount - 1) > vertexCount) {
				JE_ERROR("dest primitive starting index out of bounds, vertexBuffer=%p, index=%d, vertexCount=%d", (void*)vertexBuffer, destPrimitiveVertexIndex, vertexCount);
				continue;
			}

			for (int j = 0; j < primitiveVertexCount; j++) {
				int srcVertexIndex = srcPrimitiveVertexIndex + j;
				int destVertexIndex = destPrimitiveVertexIndex + j;
				vertices[destVertexIndex] = unsortedVertices[srcVertexIndex];
			}
		}
	}

	jeArray_destroy(&sortKeysBuffer);
	jeArray_destroy(&unsortedPrimitivesBuffer);

	return ok;
}
void jeVertexBuffer_pushPrimitive(struct jeVertexBuffer* vertexBuffer, const struct jeVertex* vertices, int primitiveType) {
	JE_TRACE("vertexBuffer=%p, primitiveType=%d, vertices=%s", (void*)vertexBuffer, primitiveType, jePrimitiveType_getDebugString(vertices, primitiveType));

	struct jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT];
	switch (primitiveType) {
		case JE_PRIMITIVE_TYPE_POINTS: {
			jeVertex_createPointQuad(quadVertices, vertices);
			jeArray_push(&vertexBuffer->vertices, (const void*)&quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_LINES: {
			jeVertex_createLineQuad(quadVertices, vertices);
			jeArray_push(&vertexBuffer->vertices, (const void*)&quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_SPRITES: {
			jeVertex_createSpriteQuad(quadVertices, vertices);
			jeArray_push(&vertexBuffer->vertices, (const void*)&quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			jeArray_push(&vertexBuffer->vertices, (const void*)vertices, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_QUADS: {
			jeArray_push(&vertexBuffer->vertices, (const void*)vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
			break;
		}
		default: {
			JE_WARN("unrecognized type, primitive=%d", primitiveType);
			break;
		}
	}
}

bool jeImage_create(struct jeImage* image, int width, int height, struct jeColorRGBA32 fillColor) {
	JE_TRACE("image=%p", (void*)image);

	bool ok = true;

	image->height = width;
	image->width = height;

	ok = ok && jeArray_create(&image->buffer, sizeof(struct jeColorRGBA32));
	ok = ok && jeArray_setCount(&image->buffer, width * height);

	if (ok) {
		struct jeColorRGBA32* pixels = (struct jeColorRGBA32*)image->buffer.data;
		for (int i = 0; i < image->buffer.count; i++) {
			pixels[i] = fillColor;
		}
	}

	return ok;
}
bool jeImage_createFromFile(struct jeImage* image, const char* filename) {
	JE_DEBUG("image=%p, filename=%s", (void*)image, filename);

	bool ok = true;

	image->height = 0;
	image->width = 0;

	jeArray_create(&image->buffer, sizeof(struct jeColorRGBA32));

	png_image pngImage;
	memset((void*)&pngImage, 0, sizeof(pngImage));
	pngImage.version = PNG_IMAGE_VERSION;

	if (ok) {
		if (png_image_begin_read_from_file(&pngImage, filename) == 0) {
			JE_WARN("png_image_begin_read_from_file() failed with filename=%s", filename);
			ok = false;
		}
	}

	if (ok) {
		pngImage.format = PNG_FORMAT_RGBA;

		ok = ok && jeArray_setCount(&image->buffer, PNG_IMAGE_SIZE(pngImage) / sizeof(struct jeColorRGBA32));
	}

	if (ok) {
		if (png_image_finish_read(&pngImage, /*background*/ NULL, image->buffer.data, /*row_stride*/ 0, /*colormap*/ NULL) == 0) {
			JE_WARN("png_image_finish_read() failed with filename=%s", filename);
			ok = false;
		}
	}

	if (ok) {
		image->width = pngImage.width;
		image->height = pngImage.height;

		JE_DEBUG("completed, filename=%s, width=%d, height=%d", filename, image->width, image->height);
	}

	if (!ok) {
		jeImage_destroy(image);
	}

	png_image_free(&pngImage);

	return ok;
}
void jeImage_destroy(struct jeImage* image) {
	JE_TRACE("image=%p", (void*)image);

	jeArray_destroy(&image->buffer);

	image->height = 0;
	image->width = 0;
}

void jeRendering_runTests() {
#if JE_DEBUGGING
	JE_DEBUG(" ");

	struct jeVertex vertices[JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT];
	JE_ASSERT(jeVertex_getDebugString(vertices) != NULL);
	for (int i = 0; i < (int)(sizeof(struct jeVertex) / sizeof(float)); i++) {
		((float*)vertices)[i] = 1.0f / 3.0f;
	}
	JE_ASSERT(jeVertex_getDebugString(vertices) != NULL);
	JE_ASSERT(strlen(jeVertex_getDebugString(vertices)) < JE_VERTEX_DEBUG_STRING_BUFFER_SIZE);

	for (int i = 1; i < JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT; i++) {
		vertices[i] = vertices[0];
	}
	JE_ASSERT(jeVertex_arraygetDebugString(vertices, JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT) != NULL);
	JE_ASSERT(strlen(jeVertex_arraygetDebugString(vertices, JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT)) < JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE);

	struct jeVertex primitiveVertices[JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT];
	memset((void*)primitiveVertices, 0, sizeof(primitiveVertices));
	jeVertex_createPointQuad(vertices, primitiveVertices);
	jeVertex_createLineQuad(vertices, primitiveVertices);
	jeVertex_createSpriteQuad(vertices, primitiveVertices);

	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_POINTS) > 0);
	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_LINES) > 0);
	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_SPRITES) > 0);
	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_TRIANGLES) > 0);
	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_QUADS) > 0);
	JE_ASSERT(jePrimitiveType_getDebugString(vertices, JE_PRIMITIVE_TYPE_QUADS) != NULL);
	JE_ASSERT(strlen(jePrimitiveType_getDebugString(vertices, JE_PRIMITIVE_TYPE_QUADS)) < JE_PRIMITIVE_TYPE_DEBUG_STRING_BUFFER_SIZE);

	struct jeVertexBuffer vertexBuffer;
	JE_ASSERT(jeVertexBuffer_create(&vertexBuffer));
	JE_ASSERT(jeVertexBuffer_sort(&vertexBuffer, JE_PRIMITIVE_TYPE_TRIANGLES));

	jeVertexBuffer_pushPrimitive(&vertexBuffer, vertices, JE_PRIMITIVE_TYPE_QUADS);
	JE_ASSERT(jeVertexBuffer_sort(&vertexBuffer, JE_PRIMITIVE_TYPE_TRIANGLES));
	JE_ASSERT(vertexBuffer.vertices.count == JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);

	vertices[0].x = 1;
	vertices[0].y = 1;
	vertices[0].z = 1;
	vertices[3].x = 1;
	vertices[3].z = 1;
	jeVertexBuffer_pushPrimitive(&vertexBuffer, vertices, JE_PRIMITIVE_TYPE_QUADS);
	JE_ASSERT(vertexBuffer.vertices.count == (JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT * 2));
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, 0))->x == 0);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, 0))->y == 0);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, 0))->z == 0);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->x == 1);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->y == 1);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->z == 1);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT + JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT))->y == 0);

	JE_ASSERT(jeVertexBuffer_sort(&vertexBuffer, JE_PRIMITIVE_TYPE_TRIANGLES));
	JE_ASSERT(vertexBuffer.vertices.count == (JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT * 2));
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, 0))->x == 1);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, 0))->y == 1);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, 0))->z == 1);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT))->y == 0);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->x == 0);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->y == 0);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->z == 0);

	jeVertexBuffer_reset(&vertexBuffer);
	JE_ASSERT(vertexBuffer.vertices.count == 0);
	jeVertexBuffer_destroy(&vertexBuffer);

	struct jeImage image;
	const struct jeColorRGBA32 white = {0xFF, 0xFF, 0xFF, 0xFF};
	JE_ASSERT(jeImage_create(&image, 16, 16, white));
	jeImage_destroy(&image);
#endif
}
