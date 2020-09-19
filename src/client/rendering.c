#include "stdafx.h"
#include "rendering.h"
#include "debug.h"
#include "container.h"


int jePrimitiveType_getVertexCount(jePrimitiveType primitiveType) {
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
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			vertexCount = JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT;
			break;
		}
		case JE_PRIMITIVE_TYPE_SPRITES: {
			vertexCount = JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT;
			break;
		}
	}

	return vertexCount;
}

const char* jeVertex_toDebugString(const jeVertex* vertex) {
	static char buffer[512];

	memset((void*)buffer, 0, sizeof(buffer));
	sprintf(
		buffer,
		"x=%.2f, y=%.2f, z=%.2f, r=%.2f, g=%.2f, b=%.2f, a=%.2f, u=%.2f, v=%.2f",
		vertex->x, vertex->y, vertex->z, vertex->r, vertex->g, vertex->b, vertex->a, vertex->u, vertex->v
	);

	return buffer;
}
const char* jeVertex_arrayToDebugString(const jeVertex* vertices, int vertexCount) {
	static char buffer[2048];

	memset((void*)buffer, 0, sizeof(buffer));

	int printedVertexCount = vertexCount;
	if (printedVertexCount > 4) {
		printedVertexCount = 4;
	}

	int bufferSize = 0;
	for (int i = 0; i < printedVertexCount; i++) {
		if (i > 0) {
			bufferSize += sprintf(buffer + bufferSize, ", ");
		}

		bufferSize += sprintf(buffer + bufferSize, "vertex[%d]={%s}", i, jeVertex_toDebugString(&vertices[i]));
	}

	if (printedVertexCount < vertexCount) {
		bufferSize += sprintf(buffer + bufferSize, ", ...");
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
	if (sortKeyA->z < sortKeyB->z) {
		result = 1;
	} else if (sortKeyA->z > sortKeyB->z) {
		result = -1;
	} else if (sortKeyA->index < sortKeyB->index) {
		result = 1;
	} else if (sortKeyA->index > sortKeyB->index) {
		result = -1;
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

	JE_TRACE("vertexBuffer=%p, vertexCount=%d, primitiveCount=%d", vertexCount, primitiveCount);

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
			int vertexIndex = primitiveVertexCount * i;
			sortKeys[i].z = vertices[vertexIndex].z;
			sortKeys[i].index = vertexIndex;
		}

		/*Sort the sort keys array.  Uses both index and depth in order for sort to be stable*/
		qsort(sortKeys, primitiveCount, sizeof(jePrimitiveSortKey), jePrimitiveSortKey_less);

		/*Sort vertices using sort key order*/
		for (int i = 0; i < primitiveCount; i++) {
			int primitiveVertexIndex = sortKeys[i].index;
			if ((primitiveVertexIndex + primitiveVertexCount - 1) > vertexCount) {
				JE_ERROR("primitive starting index out of bounds, vertexBuffer=%p, index=%d, vertexCount=%d", vertexBuffer, primitiveVertexIndex, vertexCount);
				continue;
			}

			for (int j = 0; j < primitiveVertexCount; j++) {
				int vertexIndex = primitiveVertexIndex + j;
				vertices[vertexIndex] = unsortedVertices[vertexIndex];
			}
		}
	}

	jeBuffer_destroy(&sortKeysBuffer);
	jeBuffer_destroy(&unsortedPrimitivesBuffer);

	return ok;
}
void jeVertexBuffer_pushPrimitive(jeVertexBuffer* vertexBuffer, const jeVertex* vertices, jePrimitiveType primitiveType) {
	JE_TRACE("vertexBuffer=%p, primitiveType=%d", vertexBuffer, primitiveType);

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
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			jeBuffer_push(&vertexBuffer->vertices, (const void*)vertices, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_SPRITES: {
			jeVertex_createSpriteQuad(quadVertices, vertices);
			jeBuffer_push(&vertexBuffer->vertices, (const void*)&quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
			break;
		}
		default: {
			JE_WARN("unrecognized type, primitive=%d", primitiveType);
			break;
		}
	}
}
