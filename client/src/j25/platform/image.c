#include <j25/platform/image.h>

#include <j25/core/common.h>
#include <j25/core/container.h>

#include <string.h>

#include <png.h>

bool jeImage_create(struct jeImage* image, uint32_t width, uint32_t height, struct jeColorRGBA32 fillColor) {
	JE_TRACE("image=%p", (void*)image);

	bool ok = true;

	if (image == NULL) {
		JE_ERROR("image=NULL");
		ok = false;
	}

	if (image != NULL) {
		memset((void*)image, 0, sizeof(struct jeImage));
	}

	if (ok) {
		image->height = width;
		image->width = height;
	}

	ok = ok && jeArray_create(&image->buffer, sizeof(struct jeColorRGBA32));
	ok = ok && jeArray_setCount(&image->buffer, width * height);

	if (ok) {
		struct jeColorRGBA32* pixels = (struct jeColorRGBA32*)image->buffer.data;
		for (uint32_t i = 0; i < image->buffer.count; i++) {
			pixels[i] = fillColor;
		}
	}

	return ok;
}
bool jeImage_createFromPNGFile(struct jeImage* image, const char* filename) {
	JE_DEBUG("image=%p, filename=%s", (void*)image, filename);

	bool ok = true;

	if (image == NULL) {
		JE_ERROR("image=NULL");
		ok = false;
	}

	if (ok) {
		image->height = 0;
		image->width = 0;
	}

	ok = ok && jeArray_create(&image->buffer, sizeof(struct jeColorRGBA32));

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

		ok = ok && jeArray_setCount(&image->buffer, PNG_IMAGE_SIZE(pngImage) / sizeof(struct jeColorRGBA32));
	}

	if (ok) {
		if (png_image_finish_read(
				&pngImage, /*background*/ NULL, image->buffer.data, /*row_stride*/ 0, /*colormap*/ NULL) == 0) {
			JE_WARN("png_image_finish_read() failed with filename=%s", filename);
			ok = false;
		}
	}

	if (ok) {
		image->width = pngImage.width;
		image->height = pngImage.height;

		JE_DEBUG("completed, filename=%s, width=%u, height=%u", filename, image->width, image->height);
	}

	if (!ok && (image != NULL)) {
		jeImage_destroy(image);
		image = NULL;
	}

	png_image_free(&pngImage);

	return ok;
}
void jeImage_destroy(struct jeImage* image) {
	JE_TRACE("image=%p", (void*)image);

	if (image != NULL) {
		jeArray_destroy(&image->buffer);

		image->height = 0;
		image->width = 0;
	}
}

void jeImage_runTests() {
#if JE_DEBUGGING
	JE_DEBUG(" ");

	struct jeImage image;
	const struct jeColorRGBA32 white = {0xFF, 0xFF, 0xFF, 0xFF};
	JE_ASSERT(jeImage_create(&image, 16, 16, white));
	jeImage_destroy(&image);
#endif
}
