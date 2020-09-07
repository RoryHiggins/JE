/*Precompiled header*/
#if !defined(JE_STDAFX_H)
#define JE_STDAFX_H

#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <luajit-2.0/lua.h>
#include <luajit-2.0/lualib.h>
#include <luajit-2.0/lauxlib.h>
#ifdef __cplusplus
} /*extern "C"*/
#endif

#include <zlib.h>
#include <png.h>

#define GLEW_STATIC
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <gl/glew.h>
#include <gl/glu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

#define JE_TRUE 1
#define JE_FALSE 0
typedef unsigned char jeBool;

#endif
