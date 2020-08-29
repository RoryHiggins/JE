#if !defined(JE_IMAGE_H)
#define JE_IMAGE_H

#include "core.h"


typedef struct jeImage jeImage;

struct jeImage {
	void* buffer;
	unsigned width;
	unsigned height;
};

void jeImage_destroy(jeImage* image);
bool jeImage_createFromFile(jeImage* image, char const* filename);

#endif
