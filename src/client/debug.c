#include "stdafx.h"
#include "debug.h"


void jeLogger_logPrefixImpl(const char* label, const char* file, unsigned line) {
	fprintf(stdout, "[%s %s:%d] ", label, file, line);
}
void jeLogger_logImpl(const char* formatStr, ...) {
	va_list args;

	va_start(args, formatStr);
	vfprintf(stdout, formatStr, args);
	va_end(args);

	fputc('\n', stdout);
	fflush(stdout);
}
void jeLogger_discardLogImpl(const char* formatStr, ...) {
	JE_MAYBE_UNUSED(formatStr);
}
