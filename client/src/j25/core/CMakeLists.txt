target_sources(
	j25
	PUBLIC
	"common.h"
	"container.h"
)

target_sources(
	j25
	PRIVATE
	"common.c"
	"container.c"
)

target_precompile_headers(
	j25
	PUBLIC
	"common.h"
)
