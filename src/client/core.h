#if !defined(JE_CORE_H)
#define JE_CORE_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

#include <luajit-2.0/lua.h>
#include <luajit-2.0/lualib.h>
#include <luajit-2.0/lauxlib.h>

#include <zlib.h>
#include <png.h>
	
#if defined(__cplusplus)
}
#endif

#define GLEW_STATIC
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <gl/glew.h>
#include <gl/glu.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

#define JE_LOG_CONTEXT __FILE__, __LINE__
#define JE_LOG_LOG_LABEL "LOG"
#define JE_LOG_ERR_LABEL "ERR"
#if defined(NDEBUG)
#define JE_LOG jeLog_discardLogImpl
#define JE_ERR jeLog_discardLogImpl
#else
#define JE_LOG jeLog_logPrefixImpl(JE_LOG_LOG_LABEL, JE_LOG_CONTEXT); jeLog_logImpl
#define JE_ERR jeLog_logPrefixImpl(JE_LOG_ERR_LABEL, JE_LOG_CONTEXT); jeLog_logImpl
#endif

#if !defined(__cplusplus) && (!defined(__bool_true_false_are_defined) || !__bool_true_false_are_defined)
#define true 1
#define false 0

typedef unsigned char bool;
#endif

void jeLog_logPrefixImpl(const char* label, const char* file, unsigned line);
void jeLog_logImpl(const char* formatStr, ...);
void jeLog_discardLogImpl(const char* formatStr, ...);

#endif
