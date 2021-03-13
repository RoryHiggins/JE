function(je_add_compile_link_options)
	add_compile_options(${ARGV})
	add_link_options(${ARGV})
endfunction()

if(CMAKE_BUILD_TYPE STREQUAL "RELEASE")
	SET(JE_BUILD_TARGET "RELEASE" CACHE STRING "JEngine build target")
else()
	SET(JE_BUILD_TARGET "DEVELOPMENT" CACHE STRING "JEngine build target")
endif()

project(JEngine LANGUAGES C)
cmake_minimum_required(VERSION 3.16)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS OFF)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
add_compile_definitions(JE_BUILD_${JE_BUILD_TARGET})

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	je_add_compile_link_options(-Wall -Wextra -pedantic -Werror=vla -Winvalid-pch -fno-exceptions)

	if((JE_BUILD_TARGET STREQUAL "DEVELOPMENT") OR (JE_BUILD_TARGET STREQUAL "DEBUG") OR (JE_BUILD_TARGET STREQUAL "TRACE"))
		je_add_compile_link_options(-O0)
	endif()

	if((JE_BUILD_TARGET STREQUAL "DEBUG") OR (JE_BUILD_TARGET STREQUAL "TRACE"))
		je_add_compile_link_options(-g3 -ggdb3 -fno-inline-functions -fno-omit-frame-pointer -fexceptions)

		if(NOT WIN32)
			je_add_compile_link_options(-fsanitize=address,leak,undefined -fsanitize-recover=address -fsanitize-address-use-after-scope -fstack-protector)
		endif()
	endif()

	if((JE_BUILD_TARGET STREQUAL "PROFILED") OR (JE_BUILD_TARGET STREQUAL "RELEASE"))
		je_add_compile_link_options(-fno-exceptions -Os -s -ffast-math -flto -fwhole-program -static -static-libstdc++ -static-libgcc)
	endif()

	if(JE_BUILD_TARGET STREQUAL "PROFILED")
		je_add_compile_link_options(-pg -fno-inline-functions -fno-omit-frame-pointer)
	endif()

endif()

# for compatibility with old SDL2 package (sets SDL2_LIBRARIES) and new (sets SDL2::*)
if ("${SDL2_LIBRARIES}" STREQUAL "")
	set(SDL2_LIBRARIES SDL2::SDL2main SDL2::SDL2)
endif()

set(JENGINE_DEPENDENCIES ${SDL2_LIBRARIES} OpenGL::GL OpenGL::GLU GLEW::glew PNG::PNG ZLIB::ZLIB luajit-5.1 m)
if(MINGW)
	list(PREPEND JENGINE_DEPENDENCIES mingw32)
endif()

add_executable(j25_client
	client/src/j25/core/debug.c
	client/src/j25/core/container.c
	client/src/j25/platform/rendering.c
	client/src/j25/platform/window.c
	client/src/j25/client/client.c)

target_include_directories(j25_client PUBLIC client/src ${SDL2_INCLUDE_DIRS})
target_link_libraries(j25_client PRIVATE ${JENGINE_DEPENDENCIES})
# target_precompile_headers(j25_client PRIVATE client/src/j25/stdafx.h)
set_target_properties(j25_client PROPERTIES UNITY_BUILD_MODE GROUP)

set_source_files_properties(
	client/src/j25/core/debug.c
	client/src/j25/core/container.c
	PROPERTIES UNITY_GROUP "JENGINE_CORE_UNITY")
set_source_files_properties(
	client/src/j25/platform/rendering.c
	client/src/j25/platform/window.c
	PROPERTIES UNITY_GROUP "JENGINE_PLATFORM_UNITY")
