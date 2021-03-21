#include <j25/platform/rendering.h>

#include <j25/core/container.h>
#include <j25/core/debug.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#define JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES 16
#define JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE (128 * JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES)

#define JE_PRIMITIVE_TYPE_DEBUG_STRING_BUFFER_SIZE JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE

/* Sort key which preserves both depth and order*/
struct jePrimitiveSortKey {
	float z;
	uint32_t index;
};

int jePrimitiveSortKey_less(const void* rawSortKeyA, const void* rawSortKeyB);

bool jePrimitiveType_getValid(uint32_t primitiveType) {
	bool isValid = true;

	if (primitiveType == JE_PRIMITIVE_TYPE_UNKNOWN) {
		isValid = false;
	}
	if (primitiveType > JE_PRIMITIVE_TYPE_COUNT) {
		isValid = false;
	}

	return isValid;
}
uint32_t jePrimitiveType_getVertexCount(uint32_t primitiveType) {
	JE_TRACE("primitiveType=%u", primitiveType);

	uint32_t vertexCount = 1;
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
			JE_ERROR("unexpected primitiveType, primitiveType=%u", primitiveType);
			vertexCount = 1;
			break;
		}
	}

	return vertexCount;
}
int jePrimitiveSortKey_less(const void* rawSortKeyA, const void* rawSortKeyB) {
	const struct jePrimitiveSortKey* sortKeyA = (const struct jePrimitiveSortKey*)rawSortKeyA;
	const struct jePrimitiveSortKey* sortKeyB = (const struct jePrimitiveSortKey*)rawSortKeyB;

	bool ok = true;

	if (JE_DEBUGGING) {
		if (sortKeyA == NULL) {
			JE_ERROR("sortKeyA=NULL");
			ok = false;
		}

		if (sortKeyB == NULL) {
			JE_ERROR("sortKeyB=NULL");
			ok = false;
		}
	}

	int result = 0;
	if (ok) {
		if (sortKeyA->z > sortKeyB->z) {
			result = -1;
		} else if (sortKeyA->z < sortKeyB->z) {
			result = 1;
		} else if (sortKeyA->index < sortKeyB->index) {
			result = -1;
		} else if (sortKeyA->index > sortKeyB->index) {
			result = 1;
		}
	}

	return result;
}

const char* jeVertex_getDebugString(const struct jeVertex* vertex) {
	if (vertex == NULL) {
		JE_ERROR("vertex=NULL");

		static const struct jeVertex defaultVertex = {0};
		vertex = &defaultVertex;
	}

	return je_temp_buffer_format(
		"x=%.2f, y=%.2f, z=%.2f, r=%.2f, g=%.2f, b=%.2f, a=%.2f, u=%.2f, v=%.2f",
		vertex->x,
		vertex->y,
		vertex->z,
		vertex->r,
		vertex->g,
		vertex->b,
		vertex->a,
		vertex->u,
		vertex->v);
}
const char* jeVertex_arrayGetDebugString(const struct jeVertex* vertices, uint32_t vertexCount) {
	JE_TRACE("vertices=%p, vertexCount=%u", (void*)vertices, vertexCount);

	if (vertices == NULL) {
		JE_ERROR("vertices=NULL");
		vertexCount = 0;
	}

	static char buffer[JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE];
	memset((void*)buffer, 0, sizeof(buffer));

	uint32_t printedVertexCount = vertexCount;
	if (printedVertexCount > JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES) {
		printedVertexCount = JE_VERTEX_ARRAY_DEBUG_STRING_MAX_VERTICES;
	}

	uint32_t bufferOffset = 0;
	for (uint32_t i = 0; i < printedVertexCount; i++) {
		if (i > 0) {
			bufferOffset += (uint32_t)snprintf(
				buffer + bufferOffset, (size_t)(JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE - bufferOffset), ", ");
		}

		bufferOffset += (uint32_t)snprintf(
			buffer + bufferOffset,
			(size_t)(JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE - bufferOffset),
			"vertex[%u]={%s}",
			i,
			jeVertex_getDebugString(&vertices[i]));
	}

	if (printedVertexCount < vertexCount) {
		bufferOffset += (uint32_t)snprintf(
			buffer + bufferOffset, (size_t)(JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE - bufferOffset), ", ...");
	}

	JE_MAYBE_UNUSED(bufferOffset);

	return buffer;
}
const char* jeVertex_primitiveGetDebugString(const struct jeVertex* vertices, uint32_t primitiveType) {
	JE_TRACE("vertices=%p, primitiveType=%u", (void*)vertices, primitiveType);

	uint32_t primitiveVertexCount = jePrimitiveType_getVertexCount(primitiveType);
	return jeVertex_arrayGetDebugString(vertices, primitiveVertexCount);
}
void jeVertex_createPointQuad(struct jeVertex* quadVertices, const struct jeVertex* pointVertices) {
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

	bool ok = true;

	if (quadVertices == NULL) {
		JE_ERROR("quadVertices=NULL");
		ok = false;
	}

	if (pointVertices == NULL) {
		JE_ERROR("pointVertices=NULL");
		ok = false;
	}

	if (ok) {
		for (uint32_t i = 0; i < JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT; i++) {
			quadVertices[i] = *pointVertices;
		}

		/*Give the triangles actual width, as OpenGL won't render degenerate triangles*/
		static const float pointWidth = 1.0F;
		quadVertices[2].x += pointWidth;
		quadVertices[3].x += pointWidth;
		quadVertices[5].x += pointWidth;

		quadVertices[1].y += pointWidth;
		quadVertices[4].y += pointWidth;
		quadVertices[5].y += pointWidth;
	}
}
void jeVertex_createLineQuad(struct jeVertex* quadVertices, const struct jeVertex* lineVertices) {
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

	bool ok = true;

	if (quadVertices == NULL) {
		JE_ERROR("quadVertices=NULL");
		ok = false;
	}

	if (lineVertices == NULL) {
		JE_ERROR("lineVertices=NULL");
		ok = false;
	}

	if (ok) {
		for (uint32_t i = 0; i < JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT; i++) {
			quadVertices[i] = lineVertices[0];
		}

		quadVertices[2] = lineVertices[1];
		quadVertices[3] = lineVertices[1];
		quadVertices[5] = lineVertices[1];

		/*Give the triangles actual width, as OpenGL won't render degenerate triangles*/
		static const float lineWidth = 1.0F;
		float lengthX = fabsf(lineVertices[1].x - lineVertices[0].x);
		float lengthY = fabsf(lineVertices[1].y - lineVertices[0].y);
		float isHorizontalLine = lineWidth * (float)(lengthX > lengthY);
		float isVerticalLine = lineWidth * (float)(lengthX <= lengthY);
		quadVertices[1].x += isVerticalLine;
		quadVertices[1].y += isHorizontalLine;
		quadVertices[4].x += isVerticalLine;
		quadVertices[4].y += isHorizontalLine;
		quadVertices[5].x += isVerticalLine;
		quadVertices[5].y += isHorizontalLine;
	}
}
void jeVertex_createSpriteQuad(struct jeVertex* quadVertices, const struct jeVertex* spriteVertices) {
	/* Render sprite as two triangles represented by 6 clockwise vertices */

	JE_TRACE("quadVertices=%p, spriteVertices=%p", (void*)quadVertices, (void*)spriteVertices);

	bool ok = true;

	if (quadVertices == NULL) {
		JE_ERROR("quadVertices=NULL");
		ok = false;
	}

	if (spriteVertices == NULL) {
		JE_ERROR("spriteVertices=NULL");
		ok = false;
	}

	if (ok) {
		for (uint32_t i = 0; i < JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT; i++) {
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
}

bool jeVertexBuffer_create(struct jeVertexBuffer* vertexBuffer) {
	JE_TRACE("vertexBuffer=%p", (void*)vertexBuffer);

	bool ok = true;

	if (vertexBuffer == NULL) {
		JE_ERROR("vertexBuffer=NULL");
		ok = false;
	}

	if (vertexBuffer != NULL) {
		memset((void*)vertexBuffer, 0, sizeof(struct jeVertexBuffer));
	}

	if (ok) {
		ok = jeArray_create(&vertexBuffer->vertices, sizeof(struct jeVertex));
	}

	return ok;
}
void jeVertexBuffer_destroy(struct jeVertexBuffer* vertexBuffer) {
	JE_TRACE("vertexBuffer=%p", (void*)vertexBuffer);

	if (vertexBuffer != NULL) {
		jeArray_destroy(&vertexBuffer->vertices);
		vertexBuffer = NULL;
	}
}
void jeVertexBuffer_reset(struct jeVertexBuffer* vertexBuffer) {
	bool ok = true;

	if (vertexBuffer == NULL) {
		JE_ERROR("vertexBuffer=NULL");
		ok = false;
	}

	if (ok) {
		jeArray_setCount(&vertexBuffer->vertices, 0);
	}
}
bool jeVertexBuffer_sort(struct jeVertexBuffer* vertexBuffer, uint32_t primitiveType) {
	bool ok = true;

	if (vertexBuffer == NULL) {
		JE_ERROR("vertexBuffer=NULL");
		ok = false;
	}

	if (jePrimitiveType_getValid(primitiveType) == false) {
		JE_ERROR("invalid primitiveType, primitiveType=%u", primitiveType);
		ok = false;
	}

	uint32_t primitiveVertexCount = jePrimitiveType_getVertexCount(primitiveType);
	uint32_t vertexCount = 0;
	uint32_t primitiveCount = 0;
	struct jeVertex* vertices = NULL;

	if (ok) {
		vertexCount = vertexBuffer->vertices.count;
		primitiveCount = vertexCount / primitiveVertexCount;
		vertices = (struct jeVertex*)vertexBuffer->vertices.data;

		if (vertices == NULL) {
			JE_ERROR("vertices=NULL");
			ok = false;
		}
	}

	JE_TRACE("vertexBuffer=%p, vertexCount=%u, primitiveCount=%u", (void*)vertexBuffer, vertexCount, primitiveCount);

	struct jeArray unsortedPrimitivesBuffer;
	ok = ok && jeArray_create(&unsortedPrimitivesBuffer, sizeof(struct jeVertex));
	ok = ok && jeArray_setCount(&unsortedPrimitivesBuffer, vertexCount);
	const struct jeVertex* unsortedVertices = NULL;

	if (ok) {
		unsortedVertices = (const struct jeVertex*)unsortedPrimitivesBuffer.data;

		if (unsortedVertices == NULL) {
			JE_ERROR("unsortedVertices=NULL");
			ok = false;
		}
	}

	if (ok) {
		memcpy((void*)unsortedVertices, (const void*)vertices, sizeof(struct jeVertex) * vertexCount);
	}

	struct jeArray sortKeysBuffer;
	if (!jeArray_create(&sortKeysBuffer, sizeof(struct jePrimitiveSortKey))) {
		ok = false;
	}

	ok = ok && jeArray_setCount(&sortKeysBuffer, primitiveCount);
	struct jePrimitiveSortKey* sortKeys = (struct jePrimitiveSortKey*)sortKeysBuffer.data;

	if (ok) {
		/*Generate sort keys array from vertices array*/
		for (uint32_t i = 0; i < primitiveCount; i++) {
			uint32_t primitiveVertexIndex = primitiveVertexCount * i;
			sortKeys[i].z = vertices[primitiveVertexIndex].z;
			sortKeys[i].index = primitiveVertexIndex;
		}

		/*Sort the sort keys array.  Uses both index and depth in order for sort to be stable*/
		qsort(sortKeys, primitiveCount, sizeof(struct jePrimitiveSortKey), jePrimitiveSortKey_less);

		/*Sort vertices using sort key order*/
		for (uint32_t i = 0; i < primitiveCount; i++) {
			uint32_t srcPrimitiveVertexIndex = sortKeys[i].index;
			uint32_t destPrimitiveVertexIndex = primitiveVertexCount * i;

			if ((srcPrimitiveVertexIndex + primitiveVertexCount - 1) > vertexCount) {
				JE_ERROR(
					"src primitive starting index out of bounds, vertexBuffer=%p, index=%u, vertexCount=%u",
					(void*)vertexBuffer,
					srcPrimitiveVertexIndex,
					vertexCount);
				continue;
			}

			if ((destPrimitiveVertexIndex + primitiveVertexCount - 1) > vertexCount) {
				JE_ERROR(
					"dest primitive starting index out of bounds, vertexBuffer=%p, index=%u, vertexCount=%u",
					(void*)vertexBuffer,
					destPrimitiveVertexIndex,
					vertexCount);
				continue;
			}

			for (uint32_t j = 0; j < primitiveVertexCount; j++) {
				uint32_t srcVertexIndex = srcPrimitiveVertexIndex + j;
				uint32_t destVertexIndex = destPrimitiveVertexIndex + j;
				vertices[destVertexIndex] = unsortedVertices[srcVertexIndex];
			}
		}
	}

	jeArray_destroy(&sortKeysBuffer);
	jeArray_destroy(&unsortedPrimitivesBuffer);

	return ok;
}
void jeVertexBuffer_pushPrimitive(
	struct jeVertexBuffer* vertexBuffer, const struct jeVertex* vertices, uint32_t primitiveType) {
	JE_TRACE(
		"vertexBuffer=%p, primitiveType=%u, vertices=%s",
		(void*)vertexBuffer,
		primitiveType,
		jeVertex_primitiveGetDebugString(vertices, primitiveType));

	bool ok = true;

	if (vertexBuffer == NULL) {
		JE_ERROR("vertexBuffer=NULL");
		ok = false;
	}

	if (vertices == NULL) {
		JE_ERROR("vertices=NULL");
		ok = false;
	}

	if (jePrimitiveType_getValid(primitiveType) == false) {
		JE_ERROR("invalid primitiveType, primitiveType=%u", primitiveType);
		ok = false;
	}

	if (ok) {
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
				JE_WARN("unrecognized type, primitive=%u", primitiveType);
				break;
			}
		}
	}
}

void jeRendering_runTests() {
#if JE_DEBUGGING
	JE_DEBUG(" ");

	struct jeVertex vertices[JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT];
	JE_ASSERT(jeVertex_getDebugString(vertices) != NULL);
	for (uint32_t i = 0; i < (uint32_t)(sizeof(struct jeVertex) / sizeof(float)); i++) {
		((float*)vertices)[i] = 1.0F / 3.0F;
	}
	JE_ASSERT(jeVertex_getDebugString(vertices) != NULL);

	for (uint32_t i = 1; i < JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT; i++) {
		vertices[i] = vertices[0];
	}
	JE_ASSERT(jeVertex_arrayGetDebugString(vertices, JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT) != NULL);
	JE_ASSERT(
		strlen(jeVertex_arrayGetDebugString(vertices, JE_PRIMITIVE_TYPE_MAX_VERTEX_COUNT)) <
		JE_VERTEX_ARRAY_DEBUG_STRING_BUFFER_SIZE);

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
	JE_ASSERT(jeVertex_primitiveGetDebugString(vertices, JE_PRIMITIVE_TYPE_QUADS) != NULL);
	JE_ASSERT(
		strlen(jeVertex_primitiveGetDebugString(vertices, JE_PRIMITIVE_TYPE_QUADS)) <
		JE_PRIMITIVE_TYPE_DEBUG_STRING_BUFFER_SIZE);

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
	JE_ASSERT(
		((struct jeVertex*)jeArray_get(
			 &vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT + JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT))
			->y == 0);

	JE_ASSERT(jeVertexBuffer_sort(&vertexBuffer, JE_PRIMITIVE_TYPE_TRIANGLES));
	JE_ASSERT(vertexBuffer.vertices.count == (JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT * 2));
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, 0))->x == 1);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, 0))->y == 1);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, 0))->z == 1);
	JE_ASSERT(
		((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT))->y == 0);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->x == 0);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->y == 0);
	JE_ASSERT(((struct jeVertex*)jeArray_get(&vertexBuffer.vertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT))->z == 0);

	jeVertexBuffer_reset(&vertexBuffer);
	JE_ASSERT(vertexBuffer.vertices.count == 0);
	jeVertexBuffer_destroy(&vertexBuffer);
#endif
}
