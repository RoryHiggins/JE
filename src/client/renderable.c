#include "stdafx.h"
#include "renderable.h"
#include "debug.h"

const char* jeRenderable_toDebugString(const jeRenderable* renderable) {
	static char buffer[1024];

	memset((void*)buffer, 0, sizeof(buffer));

	sprintf(
		buffer,
		"z=%f, primitiveType=%d, "
		"x1=%f, y1=%f, x2=%f, y2=%f, x3=%f, y3=%f, "
		"r=%f, g=%f, b=%f, a=%f, "
		"u1=%f, v1=%f, u2=%f, v2=%f, u3=%f, v3=%f",
		renderable->z, renderable->primitiveType,
		renderable->x1, renderable->y1, renderable->x2, renderable->y2, renderable->x3, renderable->y3,
		renderable->r, renderable->g, renderable->b, renderable->a,
		renderable->u1, renderable->v1, renderable->u2, renderable->v2, renderable->u3, renderable->v3
	);

	return buffer;
}
int jeRenderable_less(const void* aRaw, const void* bRaw) {
	const jeRenderable* a = (const jeRenderable*)aRaw;
	const jeRenderable* b = (const jeRenderable*)bRaw;

	if (a->z == b->z) {
		return a->primitiveType < b->primitiveType;
	}
	return (a->z < b->z);
}
