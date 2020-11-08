#if !defined(JE_IMAGE_H)
#define JE_IMAGE_H

#include "stdafx.h"
#include "container.h"


typedef struct jeColor jeColor;
struct jeColor {
	Uint8 r;
	Uint8 g;
	Uint8 b;
	Uint8 a;
};
static const jeColor jeColor_none =      {0x00, 0x00, 0x00, 0x00};
static const jeColor jeColor_black =     {0x00, 0x00, 0x00, 0xFF};
static const jeColor jeColor_white =     {0xFF, 0xFF, 0xFF, 0xFF};
static const jeColor jeColor_darkGray =  {0x40, 0x40, 0x40, 0xFF};
static const jeColor jeColor_gray =      {0x80, 0x80, 0x80, 0xFF};
static const jeColor jeColor_lightGray = {0xC0, 0xC0, 0xC0, 0xFF};
static const jeColor jeColor_red =       {0xFF, 0x00, 0x00, 0xFF};
static const jeColor jeColor_green =     {0x00, 0xFF, 0x00, 0xFF};
static const jeColor jeColor_blue =      {0x00, 0x00, 0xFF, 0xFF};

typedef struct jeImage jeImage;
struct jeImage {
	int width;
	int height;
	jeHeapArray buffer;
};
void jeImage_destroy(jeImage* image);
bool jeImage_create(jeImage* image, int width, int height, jeColor fillColor);
bool jeImage_createFromFile(jeImage* image, const char* filename);

void jeImageRunTests();

#endif
