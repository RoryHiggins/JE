#include "stdafx.h"
#include "debug.h"


jeLoggerContext jeLoggerContext_create(const char* file, const char* function, int line) {
	jeLoggerContext loggerContext;

	loggerContext.file = file;
	loggerContext.function = function;
	loggerContext.line = line;

	return loggerContext;
}

void jeLogger_log(jeLoggerContext loggerContext, const char* label, const char* formatStr, ...) {
	fprintf(stdout, "[%s %s:%d] %s() ", label, loggerContext.file, loggerContext.line, loggerContext.function);

	va_list args;

	va_start(args, formatStr);
	vfprintf(stdout, formatStr, args);
	va_end(args);

	fputc('\n', stdout);
	fflush(stdout);
}