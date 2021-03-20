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
if ("${SDL2_LIBRARIES}" STREQUAL "")
	set(JE_SDL2_LIBRARIES SDL2::SDL2main SDL2::SDL2)
else()
	set(JE_SDL2_LIBRARIES "${SDL2_LIBRARIES}")
endif()
set(JE_OPENGL_LIBRARIES OpenGL::GL OpenGL::GLU GLEW::glew)

if(CMAKE_BUILD_TYPE STREQUAL "RELEASE")
	SET(JE_BUILD_TARGET "RELEASE" CACHE STRING "j25 build target")
else()
	SET(JE_BUILD_TARGET "DEVELOPMENT" CACHE STRING "j25 build target")
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


