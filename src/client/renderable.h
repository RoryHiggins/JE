#if !defined(JE_RENDERABLE_H)
#define JE_RENDERABLE_H

#include "stdafx.h"
#include "image.h"

typedef struct jeVertex jeVertex;
struct jeVertex {
	float x;
	float y;
	float z;

	float r;
	float g;
	float b;
	float a;

	float u;
	float v;
};

typedef struct jeRenderable jeRenderable;
struct jeRenderable {
	float z;  /*primary sort key, to ensure translucent objects blend correctly*/
	int primitiveType;  /*secondary sort key, to minimize number of opengl draw calls*/

	float x1;
	float y1;
	float x2;
	float y2;
	float x3;
	float y3;

	float r;
	float g;
	float b;
	float a;

	float u1;
	float v1;
	float u2;
	float v2;
	float u3;
	float v3;
};

const char* jeRenderable_toDebugString(const jeRenderable* renderable);
int jeRenderable_less(const void* aRaw, const void* bRaw);

#endif
