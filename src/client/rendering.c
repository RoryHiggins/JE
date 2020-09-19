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

	int bufferSize = 0;
	for (int i = 0; i < vertexCount; i++) {
		if (i > 0) {
			bufferSize += sprintf(buffer + bufferSize, ", ");
		}

		bufferSize += sprintf(buffer + bufferSize, "vertex[%d]={%s}", i, jeVertex_toDebugString(&vertices[i]));
	}

	return buffer;
}
void jeVertex_setPoint(jeVertex vertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT], const jeVertex point[1]) {
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
	 * NOTE: vertices are not necessarily clockwise, thus not compatible with back-face culling.
	 */

	for (int i = 0; i < JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT; i++) {
		vertices[i] = *point;
	}

	/*Give the triangles actual width, as OpenGL won't render degenerate triangles*/
	static const float pointWidth = 1.0f;
	vertices[2].x += pointWidth;
	vertices[3].x += pointWidth;
	vertices[5].x += pointWidth;

	vertices[1].y += pointWidth;
	vertices[4].y += pointWidth;
	vertices[5].y += pointWidth;
}

const char* jeTriangle_toDebugString(const jeTriangle* triangle) {
	static char buffer[2048];

	memset((void*)buffer, 0, sizeof(buffer));

	int bufferSize = 0;
	for (int i = 0; i < 3; i++) {
		if (i > 0) {
			bufferSize += sprintf(buffer + bufferSize, ", ");
		}
		bufferSize += sprintf(buffer + bufferSize, "vertex[%d]={%s}", i, jeVertex_toDebugString(&triangle->vertices[i]));
	}

	return buffer;
}
int jeTriangle_less(const void* triangleARaw, const void* triangleBRaw) {
	const jeTriangle* triangleA = (const jeTriangle*)triangleARaw;
	const jeTriangle* triangleB = (const jeTriangle*)triangleBRaw;

	int result = 0;
	if (triangleA->vertices[0].z < triangleB->vertices[0].z) {
		result = 1;
	} else if (triangleA->vertices[0].z > triangleB->vertices[0].z) {
		result = -1;
	}

	return result;
}

void jeVertexBuffer_destroy(jeVertexBuffer* vertexBuffer) {
	jeBuffer_destroy(&vertexBuffer->vertices);
}
bool jeVertexBuffer_create(jeVertexBuffer* vertexBuffer) {
	return jeBuffer_create(&vertexBuffer->vertices, sizeof(jeVertex));
}
void jeVertexBuffer_reset(jeVertexBuffer* vertexBuffer) {
	jeBuffer_setCount(&vertexBuffer->vertices, 0);
}
bool jeVertexBuffer_pushTriangles(jeVertexBuffer* vertexBuffer, const jeVertex* vertices, int vertexCount) {
	if ((vertexCount % 3) != 0) {
		JE_WARN("vertexCount not divisible by 3, vertexCount=%d", vertexCount);
	}

	return jeBuffer_push(&vertexBuffer->vertices, (const void*)vertices, vertexCount);
}
void jeVertexBuffer_pushPoint(jeVertexBuffer* vertexBuffer, const jeVertex pointVertices[1]) {
	jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT];
	jeVertex_setPoint(quadVertices, &pointVertices[0]);

	jeVertexBuffer_pushTriangles(vertexBuffer, quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
}
void jeVertexBuffer_pushLine(jeVertexBuffer* vertexBuffer, const jeVertex lineVertices[2]) {
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

	jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT];
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

	jeVertexBuffer_pushTriangles(vertexBuffer, quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
}
void jeVertexBuffer_pushSprite(jeVertexBuffer* vertexBuffer, const jeVertex spriteVertices[2]) {
	/* Render sprite as two triangles represented by 6 clockwise vertices */
	jeVertex quadVertices[JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT];
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

	jeVertexBuffer_pushTriangles(vertexBuffer, quadVertices, JE_PRIMITIVE_TYPE_QUADS_VERTEX_COUNT);
}
void jeVertexBuffer_pushPrimitive(jeVertexBuffer* vertexBuffer, const jeVertex* vertices, jePrimitiveType primitiveType) {
	switch (primitiveType) {
		case JE_PRIMITIVE_TYPE_POINTS: {
			jeVertexBuffer_pushPoint(vertexBuffer, vertices);
			break;
		}
		case JE_PRIMITIVE_TYPE_LINES: {
			jeVertexBuffer_pushLine(vertexBuffer, vertices);
			break;
		}
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			jeVertexBuffer_pushTriangles(vertexBuffer, vertices, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_SPRITES: {
			jeVertexBuffer_pushSprite(vertexBuffer, vertices);
			break;
		}
		default: {
			JE_WARN("unrecognized type, primitive=%d", primitiveType);
			break;
		}
	}
}