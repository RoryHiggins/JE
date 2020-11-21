/* Precompiled header */
#if !defined(JE_DRIVERS_STDAFX_H)
#define JE_DRIVERS_STDAFX_H

#include "../stdafx.h"

#define GLEW_STATIC
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include <GL/glu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <png.h>

#include <zlib.h>

/*
 * C and C++ have different default linkage.
 * Lua(jit) headers don't explicitly state linkage of symbols,
 * but libs are (normally) built with C, so we need to do their job for them.
 */
#if defined(__cplusplus)
extern "C" {
#endif
#include <luajit-2.1/lua.h>
#include <luajit-2.1/lualib.h>
#include <luajit-2.1/lauxlib.h>

/* Only used for version checking via LUAJIT_VERSION_NUM */
#include <luajit-2.1/luajit.h>
#if defined(__cplusplus)
} /*extern "C"*/
#endif

#endif
