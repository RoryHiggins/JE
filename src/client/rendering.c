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
const char* jeRenderable_toDebugString(const jeRenderable* renderable) {
	static char buffer[2048];

	memset((void*)buffer, 0, sizeof(buffer));

	int bufferSize = 0;
	for (int i = 0; i < JE_RENDERABLE_VERTEX_COUNT; i++) {
		if (i > 0) {
			bufferSize += sprintf(buffer + bufferSize, ", ");
		}

		bufferSize += sprintf(buffer + bufferSize, "vertex[%d]={%s}", i, jeVertex_toDebugString(&renderable->vertex[i]));
	}

	return buffer;
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
bool jeVertexBuffer_push(jeVertexBuffer* vertexBuffer, const jeVertex* vertices, int count) {
	return jeBuffer_push(&vertexBuffer->vertices, (const void*)vertices, count);
}
void jeVertexBuffer_pushPoint(jeVertexBuffer* vertexBuffer, const jeRenderable* renderable) {
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

	jeVertex vertices[JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT * 2];
	for (int i = 0; i < JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT * 2; i++) {
		vertices[i] = renderable->vertex[0];
	}

	/*Give the triangles actual width, as OpenGL won't render degenerate triangles*/
	static const float pointWidth = 1.0f;
	vertices[2].x += pointWidth;
	vertices[3].x += pointWidth;
	vertices[5].x += pointWidth;

	vertices[1].y += pointWidth;
	vertices[4].y += pointWidth;
	vertices[5].y += pointWidth;

	jeVertexBuffer_push(vertexBuffer, vertices, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT * 2);
}
void jeVertexBuffer_pushLine(jeVertexBuffer* vertexBuffer, const jeRenderable* renderable) {
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

	jeVertex vertices[JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT * 2];
	for (int i = 0; i < JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT * 2; i++) {
		vertices[i] = renderable->vertex[0];
	}

	vertices[2] = renderable->vertex[1];
	vertices[3] = renderable->vertex[1];
	vertices[5] = renderable->vertex[1];

	/*Give the triangles actual width, as OpenGL won't render degenerate triangles*/
	static const float lineWidth = 1.0f;
	float lengthX = fabs(renderable->vertex[1].x - renderable->vertex[0].x);
	float lengthY = fabs(renderable->vertex[1].y - renderable->vertex[0].y);
	float isHorizontalLine = lineWidth * (float)(lengthX > lengthY);
	float isVerticalLine = lineWidth * (float)(lengthX <= lengthY);
	vertices[1].x += isVerticalLine;
	vertices[1].y += isHorizontalLine;
	vertices[4].x += isVerticalLine;
	vertices[4].y += isHorizontalLine;
	vertices[5].x += isVerticalLine;
	vertices[5].y += isHorizontalLine;

	jeVertexBuffer_push(vertexBuffer, vertices, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT * 2);
}
void jeVertexBuffer_pushSprite(jeVertexBuffer* vertexBuffer, const jeRenderable* renderable) {
	/* Render sprite as two triangles represented by 6 clockwise vertices */
	jeVertex vertices[JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT];
	for (int i = 0; i < JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT; i++) {
		vertices[i] = renderable->vertex[0];
	}
	vertices[1].x = renderable->vertex[1].x;
	vertices[1].u = renderable->vertex[1].u;

	vertices[2].y = renderable->vertex[1].y;
	vertices[2].v = renderable->vertex[1].v;

	vertices[3].y = renderable->vertex[1].y;
	vertices[3].v = renderable->vertex[1].v;

	vertices[4].x = renderable->vertex[1].x;
	vertices[4].u = renderable->vertex[1].u;

	vertices[5].x = renderable->vertex[1].x;
	vertices[5].y = renderable->vertex[1].y;
	vertices[5].u = renderable->vertex[1].u;
	vertices[5].v = renderable->vertex[1].v;

	jeVertexBuffer_push(vertexBuffer, vertices, JE_PRIMITIVE_TYPE_SPRITES_VERTEX_COUNT);
}
void jeVertexBuffer_pushRenderable(jeVertexBuffer* vertexBuffer, const jeRenderable* renderable, jePrimitiveType primitiveType) {
	switch (primitiveType) {
		case JE_PRIMITIVE_TYPE_POINTS: {
			jeVertexBuffer_pushPoint(vertexBuffer, renderable);
			break;
		}
		case JE_PRIMITIVE_TYPE_LINES: {
			jeVertexBuffer_pushLine(vertexBuffer, renderable);
			break;
		}
		case JE_PRIMITIVE_TYPE_TRIANGLES: {
			jeVertexBuffer_push(vertexBuffer, renderable->vertex, JE_PRIMITIVE_TYPE_TRIANGLES_VERTEX_COUNT);
			break;
		}
		case JE_PRIMITIVE_TYPE_SPRITES: {
			jeVertexBuffer_pushSprite(vertexBuffer, renderable);
			break;
		}
		default: {
			JE_WARN("unrecognized type, primitive=%d", primitiveType);
			break;
		}
	}
}