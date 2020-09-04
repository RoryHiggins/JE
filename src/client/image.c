#include "stdafx.h"
#include "image.h"
#include "debug.h"


void jeImage_destroy(jeImage* image) {
	if (image->buffer != NULL) {
		free(image->buffer);
		image->buffer = NULL;
	}
}
jeBool jeImage_createFromFile(jeImage* image, char const* filename) {
	jeBool success = JE_FALSE;
	int imageSize = 0;
	png_image pngImage;

	memset((void*)&pngImage, 0, sizeof(pngImage));
	memset((void*)image, 0, sizeof(*image));

	pngImage.version = PNG_IMAGE_VERSION;

	if (png_image_begin_read_from_file(&pngImage, filename) == 0) {
		JE_ERROR("jeImage_createFromFile(): png_image_begin_read_from_file() failed with filename=%s", filename);
		goto finalize;
	}

	pngImage.format = PNG_FORMAT_RGBA;

	imageSize = PNG_IMAGE_SIZE(pngImage);
	image->buffer = malloc(imageSize);

	if (png_image_finish_read(&pngImage, /*background*/ NULL, image->buffer, /*row_stride*/ 0, /*colormap*/ NULL) == 0) {
		JE_ERROR("jeImage_createFromFile(): png_image_finish_read() failed with filename=%s", filename);
		goto finalize;
	}

	image->width = pngImage.width;
	image->height = pngImage.height;

	JE_ERROR("jeImage_createFromFile(): load complete with filename=%s, width=%d, height=%d", filename, image->width, image->height);

	success = JE_TRUE;
	finalize: {
		png_image_free(&pngImage);
	}

	return success;
}
