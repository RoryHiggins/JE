#include "stdafx.h"
#include "rendering.h"
#include "debug.h"
#include "container.h"

#define JE_VERTEX_DEBUG_STRING_BUFFER_SIZE 128
#define JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES 16
#define JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE (JE_VERTEX_DEBUG_STRING_BUFFER_SIZE * JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES)

#define JE_PRIMITIVE_TYPE_DEBUG_STRING_BUFFER_SIZE JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE


const char* jeVertex_toDebugString(const jeVertex* vertex) {
	JE_TRACE("vertex=%p", vertex);

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
const char* jeVertex_arrayToDebugString(const jeVertex* vertices, int vertexCount) {
	JE_TRACE("vertices=%p, vertexCount=%d", vertices, vertexCount);

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
			jeVertex_toDebugString(&vertices[i])
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
void jeVertex_createPointQuad(jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const jeVertex pointVertices[JE_PRIMITIVE_TYPE_POINTS_VERTEX_COUNT]) {
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

	JE_TRACE("quadVertices=%p, pointVertices=%p", quadVertices, pointVertices);

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
void jeVertex_createLineQuad(jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const jeVertex lineVertices[JE_PRIMITIVE_TYPE_LINES_VERTEX_COUNT]) {
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

	JE_TRACE("quadVertices=%p, lineVertices=%p", quadVertices, lineVertices);

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
void jeVertex_createSpriteQuad(jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const jeVertex spriteVertices[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT]) {
	/* Render sprite as two triangles represented by 6 clockwise vertices */

	JE_TRACE("quadVertices=%p, spriteVertices=%p", quadVertices, spriteVertices);

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


int jePrimitiveType_getVertexCount(jePrimitiveType primitiveType) {
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
const char* jePrimitiveType_toDebugString(const jeVertex* vertices, jePrimitiveType primitiveType) {
	JE_TRACE("vertices=%p, primitiveType=%d", vertices, primitiveType);

	int primitiveVertexCount = jePrimitiveType_getVertexCount(primitiveType);
	return jeVertex_arrayToDebugString(vertices, primitiveVertexCount);
}

/* Sort key which preserves both depth and order*/
typedef struct jePrimitiveSortKey jePrimitiveSortKey;
struct jePrimitiveSortKey {
	float z;
	int index;
};
int jePrimitiveSortKey_less(const void* rawSortKeyA, const void* rawSortKeyB) {
	const jePrimitiveSortKey* sortKeyA = (const jePrimitiveSortKey*)rawSortKeyA;
	const jePrimitiveSortKey* sortKeyB = (const jePrimitiveSortKey*)rawSortKeyB;

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

void jeVertexBuffer_destroy(jeVertexBuffer* vertexBuffer) {
	JE_TRACE("vertexBuffer=%p", vertexBuffer);

	jeBuffer_destroy(&vertexBuffer->vertices);
}
bool jeVertexBuffer_create(jeVertexBuffer* vertexBuffer) {
	JE_TRACE("vertexBuffer=%p", vertexBuffer);

	return jeBuffer_create(&vertexBuffer->vertices, sizeof(jeVertex));
}
void jeVertexBuffer_reset(jeVertexBuffer* vertexBuffer) {
	jeBuffer_setCount(&vertexBuffer->vertices, 0);
}
bool jeVertexBuffer_sort(jeVertexBuffer* vertexBuffer, jePrimitiveType primitiveType) {
	int primitiveVertexCount = jePrimitiveType_getVertexCount(primitiveType);
	int vertexCount = vertexBuffer->vertices.count;
	int primitiveCount = vertexCount / primitiveVertexCount;
	jeVertex *vertices = (jeVertex*)vertexBuffer->vertices.data;

	JE_TRACE("vertexBuffer=%p, vertexCount=%d, primitiveCount=%d", vertexBuffer, vertexCount, primitiveCount);

	bool ok = true;

	jeBuffer unsortedPrimitivesBuffer;
	ok = ok && jeBuffer_create(&unsortedPrimitivesBuffer, sizeof(jeVertex));
	ok = ok && jeBuffer_setCount(&unsortedPrimitivesBuffer, vertexCount);
	const jeVertex *unsortedVertices = (const jeVertex*)unsortedPrimitivesBuffer.data;
	memcpy((void*)unsortedVertices, (const void*)vertices, sizeof(jeVertex) * vertexCount);

	jeBuffer sortKeysBuffer;
	ok = ok && jeBuffer_create(&sortKeysBuffer, sizeof(jePrimitiveSortKey));
	ok = ok && jeBuffer_setCount(&sortKeysBuffer, primitiveCount);
	jePrimitiveSortKey *sortKeys = (jePrimitiveSortKey*)sortKeysBuffer.data;

	if (ok) {
		/*Generate sort keys array from vertices array*/
		for (int i = 0; i < primitiveCount; i++) {
			int primitiveVertexIndex = primitiveVertexCount * i;
			sortKeys[i].z = vertices[primitiveVertexIndex].z;
			sortKeys[i].index = primitiveVertexIndex;
		}

		/*Sort the sort keys array.  Uses both index and depth in order for sort to be stable*/
		qsort(sortKeys, primitiveCount, sizeof(jePrimitiveSortKey), jePrimitiveSortKey_less);

		/*Sort vertices using sort key order*/
		for (int i = 0; i < primitiveCount; i++) {
			int srcPrimitiveVertexIndex = sortKeys[i].index;
			int destPrimitiveVertexIndex = primitiveVertexCount * i;

			if ((srcPrimitiveVertexIndex + primitiveVertexCount - 1) > vertexCount) {
				JE_ERROR("src primitive starting index out of bounds, vertexBuffer=%p, index=%d, vertexCount=%d", vertexBuffer, srcPrimitiveVertexIndex, vertexCount);
				continue;
			}

			if ((destPrimitiveVertexIndex + primitiveVertexCount - 1) > vertexCount) {
				JE_ERROR("dest primitive starting index out of bounds, vertexBuffer=%p, index=%d, vertexCount=%d", vertexBuffer, destPrimitiveVertexIndex, vertexCount);
				continue;
			}

			for (int j = 0; j < primitiveVertexCount; j++) {
				int srcVertexIndex = srcPrimitiveVertexIndex + j;
				int destVertexIndex = destPrimitiveVertexIndex + j;
				vertices[destVertexIndex] = unsortedVertices[srcVertexIndex];
			}
		}
	}

	jeBuffer_destroy(&sortKeysBuffer);
	jeBuffer_destroy(&unsortedPrimitivesBuffer);

	return ok;
}
void jeVertexBuffer_pushPrimitive(jeVertexBuffer* vertexBuffer, const jeVertex* vertices, jePrimitiveType primitiveType) {
	JE_TRACE("vertexBuffer=%p, primitiveType=%d, vertices=%s", vertexBuffer, primitiveType, jePrimitiveType_toDebugString(vertices, primitiveType));

	jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT];
	switch (primitiveType) {
		case JE_PRIMITIVE_TYPE_POINTS: {
			jeVertex_createPointQuad(quadVertices, vertices);
			jeBuffer_push(&vertexBuffer->vertices, (const void*)&quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_LINES: {
			jeVertex_createLineQuad(quadVertices, vertices);
			jeBuffer_push(&vertexBuffer->vertices, (const void*)&quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_SPRITES: {
			jeVertex_createSpriteQuad(quadVertices, vertices);
			jeBuffer_push(&vertexBuffer->vertices, (const void*)&quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			jeBuffer_push(&vertexBuffer->vertices, (const void*)vertices, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_QUADS: {
			jeBuffer_push(&vertexBuffer->vertices, (const void*)vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
			break;
		}
		default: {
			JE_WARN("unrecognized type, primitive=%d", primitiveType);
			break;
		}
	}
}

void jeRenderingRunTests() {
	JE_DEBUG("");

	jeVertex vertices[JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT];
	JE_ASSERT(jeVertex_toDebugString(&vertices[0]) != NULL);
	for (int i = 0; i < (int)(sizeof(jeVertex) / sizeof(float)); i++) {
		((float*)&vertices[0])[i] = 1.0f / 3.0f;
	}
	JE_ASSERT(jeVertex_toDebugString(&vertices[0]) != NULL);
	JE_ASSERT(strnlen(jeVertex_toDebugString(&vertices[0]), JE_VERTEX_DEBUG_STRING_BUFFER_SIZE) < JE_VERTEX_DEBUG_STRING_BUFFER_SIZE);

	for (int i = 1; i < JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT; i++) {
		vertices[i] = vertices[0];
	}
	JE_ASSERT(jeVertex_arrayToDebugString(vertices, JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT) != NULL);
	JE_ASSERT(strnlen(jeVertex_arrayToDebugString(vertices, JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT), JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE) < JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE);

	jeVertex primitiveVertices[JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT];
	memset((void*)primitiveVertices, 0, sizeof(primitiveVertices));
	jeVertex_createPointQuad(vertices, primitiveVertices);
	jeVertex_createLineQuad(vertices, primitiveVertices);
	jeVertex_createSpriteQuad(vertices, primitiveVertices);

	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_POINTS) > 0);
	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_LINES) > 0);
	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_SPRITES) > 0);
	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_TRIANGLES) > 0);
	JE_ASSERT(jePrimitiveType_getVertexCount(JE_PRIMITIVE_TYPE_QUADS) > 0);
	JE_ASSERT(jePrimitiveType_toDebugString(vertices, JE_PRIMITIVE_TYPE_QUADS) != NULL);
	JE_ASSERT(strnlen(jePrimitiveType_toDebugString(vertices, JE_PRIMITIVE_TYPE_QUADS), JE_PRIMITIVE_TYPE_DEBUG_STRING_BUFFER_SIZE) < JE_PRIMITIVE_TYPE_DEBUG_STRING_BUFFER_SIZE);

	jeVertexBuffer vertexBuffer;
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
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, 0))->x == 0);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, 0))->y == 0);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, 0))->z == 0);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->x == 1);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->y == 1);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->z == 1);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT + JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT))->y == 0);

	JE_ASSERT(jeVertexBuffer_sort(&vertexBuffer, JE_PRIMITIVE_TYPE_TRIANGLES));
	JE_ASSERT(vertexBuffer.vertices.count == (JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT * 2));
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, 0))->x == 1);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, 0))->y == 1);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, 0))->z == 1);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT))->y == 0);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->x == 0);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->y == 0);
	JE_ASSERT(((jeVertex*)jeBuffer_getElement(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->z == 0);


	jeVertexBuffer_reset(&vertexBuffer);
	JE_ASSERT(vertexBuffer.vertices.count == 0);
	jeVertexBuffer_destroy(&vertexBuffer);
}