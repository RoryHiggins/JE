target_sources(
	j25
	PUBLIC
	"image.h"
	"rendering.h"
	"window.h"
)

target_sources(
	j25
	PRIVATE
	"image.c"
	"image_headless.c"
	"window.c"
	"window_headless.c"
	"rendering.c"
)
