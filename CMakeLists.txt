cmake_minimum_required(VERSION 3.16)

project(j25 LANGUAGES C)

function(je_add_compile_link_options)
	add_compile_options(${ARGV})
	add_link_options(${ARGV})
endfunction()

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

# for compatibility with both old SDL2 package (sets SDL2_LIBRARIES) and new (sets SDL2::*)
if ("${SDL2_LIBRARIES}" STREQUAL "")
	set(JE_SDL2_LIBRARIES SDL2::SDL2main SDL2::SDL2)
else()
	set(JE_SDL2_LIBRARIES "${SDL2_LIBRARIES}")
endif()

set(JE_OPENGL_LIBRARIES OpenGL::GL OpenGL::GLU GLEW::glew)

SET(JE_BUILD_TARGET "RELEASE" CACHE STRING "j25 build target")
if(JE_BUILD_TARGET STREQUAL "RELEASE")
	SET(CMAKE_BUILD_TYPE "RELEASE")
else()
	SET(CMAKE_BUILD_TYPE "DEBUG")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_C_EXTENSIONS false)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS false)
set(JE_ENGINE_LINK_MODE STATIC)

if(JE_BUILD_TARGET STREQUAL "DEVELOPMENT")
	set(JE_ENGINE_LINK_MODE SHARED)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	je_add_compile_link_options(-Wall -Wextra -pedantic -Werror=vla -Winvalid-pch -fno-exceptions)

	if((JE_BUILD_TARGET STREQUAL "DEVELOPMENT") OR (JE_BUILD_TARGET STREQUAL "DEBUG") OR (JE_BUILD_TARGET STREQUAL "TRACE"))
		je_add_compile_link_options(-O0)
	endif()

	if((JE_BUILD_TARGET STREQUAL "DEBUG") OR (JE_BUILD_TARGET STREQUAL "TRACE"))
		je_add_compile_link_options(-Og -g3 -ggdb3 -fno-inline-functions -fno-omit-frame-pointer -fexceptions)

		# unwanted feature warnings
		je_add_compile_link_options(-Wshadow -Wmissing-declarations -Wpacked -Wredundant-decls)

		# object size warnings (arbitrary 64mb limit)
		je_add_compile_link_options(-Wlarger-than=67108864 -Walloc-size-larger-than=67108864)

		# stack warnings (arbitrary divisors of 1mb; 1mb = default stack size on windows)
		je_add_compile_link_options(
			-fstack-protector -fstack-protector-all -Wstack-protector
			-Wframe-larger-than=65535 -Wstack-usage=524288)

		# extra warnings
		je_add_compile_link_options(
			-fanalyzer
			-pedantic -Walloc-zero -Warray-bounds=2 -Wcast-align=strict -Wconversion -Wundef
			-Wformat=2 -Wformat-overflow=2 -Wformat-signedness -Wformat-truncation=2
			-Wdisabled-optimization -Wfloat-conversion -Wlogical-op -Winline -Wmultichar
			-Wnull-dereference -Wstringop-overflow=4)

		if(NOT WIN32)
			je_add_compile_link_options(-fsanitize=address,leak,undefined -fsanitize-recover=address -fsanitize-address-use-after-scope -fstack-protector)
		endif()
	endif()

	if((JE_BUILD_TARGET STREQUAL "PROFILED") OR (JE_BUILD_TARGET STREQUAL "RELEASE"))
		je_add_compile_link_options(-fno-exceptions -Os -ffast-math -flto -fwhole-program -static -static-libstdc++ -static-libgcc)
	endif()

	if(JE_BUILD_TARGET STREQUAL "PROFILED")
		je_add_compile_link_options(-g -pg)
	endif()

	if(JE_BUILD_TARGET STREQUAL "RELEASE")
		je_add_compile_link_options(-s)
	endif()
endif()

add_library(j25 ${JE_ENGINE_LINK_MODE})
set_target_properties(j25 PROPERTIES LINKER_LANGUAGE C)
target_compile_definitions(j25 PUBLIC "JE_BUILD_${JE_BUILD_TARGET}")
if (WIN32 AND (JE_ENGINE_LINK_MODE STREQUAL "SHARED"))
	target_compile_definitions(j25 PUBLIC "JE_DLL")
	target_compile_definitions(j25 PRIVATE "JE_DLL_EXPORT")
endif()
if(MINGW)
	target_link_libraries(j25 PRIVATE mingw32)
endif()

target_link_libraries(j25 PRIVATE ${JE_SDL2_LIBRARIES} ${JE_OPENGL_LIBRARIES} PNG::PNG m)

add_executable(j25_client)
target_link_libraries(j25_client PRIVATE j25 ZLIB::ZLIB luajit-5.1)
set_target_properties(j25_client PROPERTIES LINK_DEPENDS_NO_SHARED true)

add_subdirectory("client")


