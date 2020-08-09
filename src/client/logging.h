#pragma once
#include "stdafx.h"

#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

#if defined(NDEBUG)
#define JE_DEBUG false
#define JE_LOG(...)
#define JE_ERR(...)
#else
#define JE_DEBUG true
#define JE_LOG(...) jeLog_logImpl("LOG", __FILE__, __LINE__, __VA_ARGS__)
#define JE_ERR(...) jeLog_logImpl("ERR", __FILE__, __LINE__, __VA_ARGS__)
#endif

void jeLog_logImpl(const char* label, const char* file, unsigned line, const char* formatStr, ...) {
	va_list args;

	fprintf(stdout, "[%s %s:%d] ", label, file, line);

	va_start(args, formatStr);
	vfprintf(stdout, formatStr, args);
	va_end(args);

	fputc('\n', stdout);
	fflush(stdout);
}
