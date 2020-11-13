#if !defined(JE_IMAGE_H)
#define JE_IMAGE_H

#include "container.h"

struct jeColor {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};
static const struct jeColor jeColor_none =      {0x00, 0x00, 0x00, 0x00};
static const struct jeColor jeColor_black =     {0x00, 0x00, 0x00, 0xFF};
static const struct jeColor jeColor_white =     {0xFF, 0xFF, 0xFF, 0xFF};
static const struct jeColor jeColor_darkGray =  {0x40, 0x40, 0x40, 0xFF};
static const struct jeColor jeColor_gray =      {0x80, 0x80, 0x80, 0xFF};
static const struct jeColor jeColor_lightGray = {0xC0, 0xC0, 0xC0, 0xFF};
static const struct jeColor jeColor_red =       {0xFF, 0x00, 0x00, 0xFF};
static const struct jeColor jeColor_green =     {0x00, 0xFF, 0x00, 0xFF};
static const struct jeColor jeColor_blue =      {0x00, 0x00, 0xFF, 0xFF};

struct jeImage {
	int width;
	int height;
	struct jeArray buffer;
};
void jeImage_destroy(struct jeImage* image);
bool jeImage_create(struct jeImage* image, int width, int height, struct jeColor fillColor);
bool jeImage_createFromFile(struct jeImage* image, const char* filename);

void jeImageRunTests();

#endif
