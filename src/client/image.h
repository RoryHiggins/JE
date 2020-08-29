#if !defined(JE_IMAGE_H)
#define JE_IMAGE_H

#include "stdafx.h"


typedef struct jeImage jeImage;

struct jeImage {
	void* buffer;
	int width;
	int height;
};

void jeImage_destroy(jeImage* image);
jeBool jeImage_createFromFile(jeImage* image, char const* filename);

#endif
