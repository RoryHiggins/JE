/*Precompiled header*/
#if !defined(JE_STDAFX_H)
#define JE_STDAFX_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * C and C++ have different default linkage.
 * Lua(jit) headers don't explicitly state linkage of symbols,
 * but libs are (normally) built with C, so we need to do their job for them.
 */
#ifdef __cplusplus
extern "C" {
#endif
#include <luajit-2.1/lua.h>
#include <luajit-2.1/lualib.h>
#include <luajit-2.1/lauxlib.h>

/* Only used for version checking via LUAJIT_VERSION_NUM */
#include <luajit-2.1/luajit.h>
#ifdef __cplusplus
} /*extern "C"*/
#endif

#include <zlib.h>
#include <png.h>

#define GLEW_STATIC
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GL/glew.h>
#include <GL/glu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

/**
 * Casts the result to void to inform the compiler that the result is not used
 * Primary use-case is to suppress unused function argument warnings
 */
#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

#endif
