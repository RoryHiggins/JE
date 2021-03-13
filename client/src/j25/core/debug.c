#include <j25/stdafx.h>

#define JE_LOG_LABEL_TRACE "trace"
#define JE_LOG_LABEL_DEBUG "debug"
#define JE_LOG_LABEL_INFO  "info "
#define JE_LOG_LABEL_WARN  "WARN "
#define JE_LOG_LABEL_ERR   "ERROR"

void jeErr() {
	/*dummy function to breakpoint errors*/
}

const char* jeLoggerLevel_getLabel(int loggerLevel) {
	const char* label = "";
	switch (loggerLevel) {
		case JE_MAX_LOG_LEVEL_TRACE: {
			label = JE_LOG_LABEL_TRACE;
			break;
		}
		case JE_MAX_LOG_LEVEL_DEBUG: {
			label = JE_LOG_LABEL_DEBUG;
			break;
		}
		case JE_MAX_LOG_LEVEL_INFO: {
			label = JE_LOG_LABEL_INFO;
			break;
		}
		case JE_MAX_LOG_LEVEL_WARN: {
			label = JE_LOG_LABEL_WARN;
			break;
		}
		case JE_MAX_LOG_LEVEL_ERR: {
			label = JE_LOG_LABEL_ERR;
			break;
		}
		default: {
			JE_WARN("unknown logger level, loggerLevel=%d", loggerLevel);
		}
	}
	return label;
}

struct jeLogger jeLogger_create(const char* file, const char* function, int line) {
	struct jeLogger logger;

	logger.file = file;
	logger.function = function;
	logger.line = line;

	return logger;
}

int jeLogger_levelOverride = JE_MAX_LOG_LEVEL;
int jeLogger_getLevel() {
	return jeLogger_levelOverride;
}
void jeLogger_setLevelOverride(int levelOverride) {
	if (levelOverride < JE_MAX_LOG_LEVEL) {
		JE_ERROR("invalid levelOverride below compiled minimum, levelOverride=%d, JE_MAX_LOG_LEVEL=%d", levelOverride, JE_MAX_LOG_LEVEL);
		levelOverride = JE_MAX_LOG_LEVEL;
	}
	jeLogger_levelOverride = levelOverride;
}
void jeLogger_log(struct jeLogger logger, int loggerLevel, const char* formatStr, ...) {
	if (jeLogger_levelOverride <= loggerLevel) {
		if (loggerLevel <= JE_MAX_LOG_LEVEL_ERR) {
			jeErr();
		}

		const char* label = jeLoggerLevel_getLabel(loggerLevel);
		fprintf(stdout, "[%s %s:%d] %s() ", label, logger.file, logger.line, logger.function);

		va_list args;
		va_start(args, formatStr);
		vfprintf(stdout, formatStr, args);
		va_end(args);

		fputc('\n', stdout);
		fflush(stdout);
	}
}
void jeLogger_assert(struct jeLogger logger, bool value, const char* expressionStr) {
	if ((jeLogger_levelOverride <= JE_MAX_LOG_LEVEL_ERR) && (!value)) {
		jeErr();
		jeLogger_log(logger, JE_MAX_LOG_LEVEL_ERR, "assertion failed, assertion=%s", expressionStr);
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
