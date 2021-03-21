#pragma once

#if !defined(JE_PLATFORM_IMAGE_H)
#define JE_PLATFORM_IMAGE_H

#include <j25/core/api.h>
#include <j25/core/container.h>

#include <stdbool.h>
#include <stdint.h>

struct jeColorRGBA32 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

struct jeImage {
	uint32_t width;
	uint32_t height;
	struct jeArray buffer;
};

JE_API_PUBLIC bool
jeImage_create(struct jeImage* image, uint32_t width, uint32_t height, struct jeColorRGBA32 fillColor);
JE_API_PUBLIC bool jeImage_createFromFile(struct jeImage* image, const char* filename);
JE_API_PUBLIC void jeImage_destroy(struct jeImage* image);

JE_API_PUBLIC void jeImage_runTests();

#endif
