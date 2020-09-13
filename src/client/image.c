#include "stdafx.h"
#include "image.h"
#include "debug.h"


void jeImage_destroy(jeImage* image) {
	JE_TRACE("");

	free(image->buffer);

	image->height = 0;
	image->width = 0;
	image->buffer = NULL;
}
bool jeImage_create(jeImage* image, const char* filename) {
	bool ok = true;

	JE_DEBUG("filename=%s", filename);
	
	image->buffer = NULL;
	image->width = 0;
	image->height = 0;

	png_image pngImage;
	memset((void*)&pngImage, 0, sizeof(pngImage));
	pngImage.version = PNG_IMAGE_VERSION;

	if (ok) {
		if (png_image_begin_read_from_file(&pngImage, filename) == 0) {
			JE_ERROR("png_image_begin_read_from_file() failed with filename=%s", filename);
			ok = false;
		}
	}

	if (ok) {
		pngImage.format = PNG_FORMAT_RGBA;

		int imageSize = PNG_IMAGE_SIZE(pngImage);
		image->buffer = malloc(imageSize);

		if (image->buffer == NULL) {
			JE_ERROR("malloc() failed with size=%s", imageSize);
			ok = false;
		}
	}

	if (ok) {
		if (png_image_finish_read(&pngImage, /*background*/ NULL, image->buffer, /*row_stride*/ 0, /*colormap*/ NULL) == 0) {
			JE_ERROR("png_image_finish_read() failed with filename=%s", filename);
			ok = false;
		}
	}

	if (ok) {
		image->width = pngImage.width;
		image->height = pngImage.height;

		JE_DEBUG("completed, filename=%s, width=%d, height=%d", filename, image->width, image->height);
	}

	if (!ok) {
		jeImage_destroy(image);
	}

	png_image_free(&pngImage);

	return ok;
}
