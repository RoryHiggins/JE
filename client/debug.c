#include "stdafx.h"
#include "debug.h"


jeLoggerLevel jeLoggerLevel_override = JE_LOG_LEVEL_TRACE;

const char* jeLoggerLevel_getLabel(jeLoggerLevel loggerLevel) {
	const char* label = "";
	switch (loggerLevel) {
		case JE_LOG_LEVEL_TRACE: {
			label = JE_LOG_LABEL_TRACE;
			break;
		}
		case JE_LOG_LEVEL_DEBUG: {
			label = JE_LOG_LABEL_DEBUG;
			break;
		}
		case JE_LOG_LEVEL_INFO: {
			label = JE_LOG_LABEL_INFO;
			break;
		}
		case JE_LOG_LEVEL_WARN: {
			label = JE_LOG_LABEL_WARN;
			break;
		}
		case JE_LOG_LEVEL_ERR: {
			label = JE_LOG_LABEL_ERR;
			break;
		}
		default: {
			JE_WARN("unknown logger level, loggerLevel=%d", loggerLevel);
		}
	}
	return label;
}

jeLoggerContext jeLoggerContext_create(const char* file, const char* function, int line) {
	jeLoggerContext loggerContext;

	loggerContext.file = file;
	loggerContext.function = function;
	loggerContext.line = line;

	return loggerContext;
}

void jeLogger_log(jeLoggerContext loggerContext, jeLoggerLevel loggerLevel, const char* formatStr, ...) {
	if (loggerLevel >= jeLoggerLevel_override) {
		const char* label = jeLoggerLevel_getLabel(loggerLevel);
		fprintf(stdout, "[%s %s:%d] %s() ", label, loggerContext.file, loggerContext.line, loggerContext.function);

		va_list args;
		va_start(args, formatStr);
		vfprintf(stdout, formatStr, args);
		va_end(args);

		fputc('\n', stdout);
		fflush(stdout);
	}
}

/* Parameters for AddressSanitizer; https://github.com/google/sanitizers/wiki/AddressSanitizerFlags */
const char *__asan_default_options() {
	return (
		"verbosity=1"
		":halt_on_error=0"
		":strict_string_checks=1"
		":detect_stack_use_after_return=1"
		":check_initialization_order=1"
		":strict_init_order=1"
		":detect_invalid_pointer_pairs=10"
		":detect_leaks=1"
	);
}
