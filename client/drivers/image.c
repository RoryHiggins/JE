#include "stdafx.h"

void jeImage_destroy(struct jeImage* image) {
	JE_TRACE("image=%p", image);

	jeArray_destroy(&image->buffer);

	image->height = 0;
	image->width = 0;
}
jeBool jeImage_create(struct jeImage* image, int width, int height, struct jeColor fillColor) {
	JE_TRACE("image=%p", image);

	jeBool ok = true;

	image->height = width;
	image->width = height;

	ok = ok && jeArray_create(&image->buffer, sizeof(struct jeColor));
	ok = ok && jeArray_setCount(&image->buffer, width * height);

	if (ok) {
		struct jeColor* pixels = (struct jeColor*)image->buffer.data;
		for (int i = 0; i < image->buffer.count; i++) {
			pixels[i] = fillColor;
		}
	}

	return ok;
}
jeBool jeImage_createFromFile(struct jeImage* image, const char* filename) {
	JE_DEBUG("image=%p, filename=%s", image, filename);

	jeBool ok = true;

	image->height = 0;
	image->width = 0;

	jeArray_create(&image->buffer, sizeof(struct jeColor));

	png_image pngImage;
	memset((void*)&pngImage, 0, sizeof(pngImage));
	pngImage.version = PNG_IMAGE_VERSION;

	if (ok) {
		if (png_image_begin_read_from_file(&pngImage, filename) == 0) {
			JE_WARN("png_image_begin_read_from_file() failed with filename=%s", filename);
			ok = false;
		}
	}

	if (ok) {
		pngImage.format = PNG_FORMAT_RGBA;

		ok = ok && jeArray_setCount(&image->buffer, PNG_IMAGE_SIZE(pngImage) / sizeof(struct jeColor));
	}

	if (ok) {
		if (png_image_finish_read(&pngImage, /*background*/ NULL, image->buffer.data, /*row_stride*/ 0, /*colormap*/ NULL) == 0) {
			JE_WARN("png_image_finish_read() failed with filename=%s", filename);
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

void jeImage_runTests() {
	JE_DEBUG("");

	struct jeImage image;
	JE_ASSERT(jeImage_create(&image, 16, 16, jeColor_white));

	jeImage_destroy(&image);
}
