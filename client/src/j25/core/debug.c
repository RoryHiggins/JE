#include <j25/core/debug.h>

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define JE_LOG_LABEL_TRACE "trace"
#define JE_LOG_LABEL_DEBUG "debug"
#define JE_LOG_LABEL_INFO  "info "
#define JE_LOG_LABEL_WARN  "WARN "
#define JE_LOG_LABEL_ERR   "ERROR"


#define JE_TEMP_BUFFER_CAPACITY 262144


void jeErr();

const char* jeLoggerLevel_getLabel(uint32_t loggerLevel);

const char *__asan_default_options();


void jeErr() {
	/* dummy function to breakpoint on an error */
}


const char* jeLoggerLevel_getLabel(uint32_t loggerLevel) {
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
			JE_WARN("unknown logger level, loggerLevel=%u", loggerLevel);
		}
	}
	return label;
}


struct jeLogger jeLogger_create(const char* file, const char* function, uint32_t line) {
	struct jeLogger logger = {0};
	logger.file = file;
	logger.function = function;
	logger.line = line;

	return logger;
}


uint32_t jeLogger_levelOverride = JE_MAX_LOG_LEVEL;
uint32_t jeLogger_getLevel() {
	return jeLogger_levelOverride;
}
void jeLogger_setLevelOverride(uint32_t levelOverride) {
	if (levelOverride < JE_MAX_LOG_LEVEL) {
		JE_ERROR("invalid levelOverride below compiled minimum, levelOverride=%u, JE_MAX_LOG_LEVEL=%d", levelOverride, JE_MAX_LOG_LEVEL);
		levelOverride = JE_MAX_LOG_LEVEL;
	}
	jeLogger_levelOverride = levelOverride;
}
void jeLogger_log(struct jeLogger logger, uint32_t loggerLevel, const char* formatStr, ...) {
	if (jeLogger_levelOverride <= loggerLevel) {
		if (loggerLevel <= JE_MAX_LOG_LEVEL_ERR) {
			jeErr();
		}

		const char* label = jeLoggerLevel_getLabel(loggerLevel);
		fprintf(stdout, "[%s %s:%u] %s() ", label, logger.file, logger.line, logger.function);

		va_list args = {0};
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


char* je_temp_buffer_allocate(uint32_t size) {
	static uint32_t currentSize = 0;
	static char buffer[JE_TEMP_BUFFER_CAPACITY] = {0};

	if ((currentSize + size) > JE_TEMP_BUFFER_CAPACITY) {
		currentSize = 0;
	}
	if (size > JE_TEMP_BUFFER_CAPACITY) {
		JE_ERROR("requested size above capacity, size=%u", size);
		size = JE_TEMP_BUFFER_CAPACITY;
	}

	char* pos = buffer + currentSize;
	memset((void*)pos, 0, size);
	currentSize += size;

	return pos;
}
char* je_temp_buffer_allocate_aligned(uint32_t size, uint32_t alignment) {
	if (alignment == 0) {
		JE_ERROR("requested 0 alignment");
		alignment = 1;
	}

	size_t unaligned = (size_t)je_temp_buffer_allocate(size + alignment);

	return (char*)(((unaligned + alignment - 1) / alignment) * alignment);
}
const char* je_temp_buffer_format(const char* format_str, ...) {
	va_list args = {0};
	va_start(args, format_str);

	va_list computeSizeArgs = {0};
	va_copy(computeSizeArgs, args);

	int computedCount = vsnprintf(/*buffer*/ NULL, 0, format_str, computeSizeArgs);
	if (computedCount < 0) {
		computedCount = 0;
	}

	uint32_t allocation_count = (uint32_t)(computedCount + 1);
	char* allocation = je_temp_buffer_allocate(allocation_count);
	vsnprintf(allocation, (size_t)allocation_count, format_str, args);

	va_end(computeSizeArgs);
	va_end(args);

	return allocation;
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
