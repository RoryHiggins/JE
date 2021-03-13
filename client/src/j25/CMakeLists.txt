add_subdirectory("core")
add_subdirectory("platform")
add_subdirectory("client")

target_sources(
	j25
	PUBLIC
	"core.h"
)
