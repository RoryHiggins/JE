#include "private_dependencies.h"
#include "image.h"
#include "debug.h"
#include "container.h"


void jeImage_destroy(jeImage* image) {
	JE_TRACE("image=%p", image);

	jeArray_destroy(&image->buffer);

	image->height = 0;
	image->width = 0;
}
bool jeImage_create(jeImage* image, int width, int height, jeColor fillColor) {
	JE_TRACE("image=%p", image);

	bool ok = true;

	image->height = width;
	image->width = height;

	ok = ok && jeArray_create(&image->buffer, sizeof(jeColor));
	ok = ok && jeArray_setCount(&image->buffer, width * height);

	if (ok) {
		jeColor* pixels = (jeColor*)image->buffer.data;
		for (int i = 0; i < image->buffer.count; i++) {
			pixels[i] = fillColor;
		}
	}

	return ok;
}
bool jeImage_createFromFile(jeImage* image, const char* filename) {
	JE_DEBUG("image=%p, filename=%s", image, filename);

	bool ok = true;

	image->height = 0;
	image->width = 0;

	jeArray_create(&image->buffer, sizeof(jeColor));

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

		ok = ok && jeArray_setCount(&image->buffer, PNG_IMAGE_SIZE(pngImage) / sizeof(jeColor));
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

void jeImageRunTests() {
	JE_DEBUG("");

	jeImage image;
	JE_ASSERT(jeImage_create(&image, 16, 16, jeColor_white));

	jeImage_destroy(&image);
}
