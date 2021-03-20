#pragma once

#if !defined(JE_CORE_DEBUG_H)
#define JE_CORE_DEBUG_H

#include <j25/core/api.h>

#include <stdbool.h>

/**
 * Casts the result to void to inform the compiler that the result is not used
 * Primary use-case is to suppress unused function argument warnings
 */
#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

#define JE_MAX_LOG_LEVEL_TRACE 0
#define JE_MAX_LOG_LEVEL_DEBUG 1
#define JE_MAX_LOG_LEVEL_INFO  2
#define JE_MAX_LOG_LEVEL_WARN  3
#define JE_MAX_LOG_LEVEL_ERR   4
#define JE_MAX_LOG_LEVEL_COUNT 5
#define JE_MAX_LOG_LEVEL_NONE  JE_MAX_LOG_LEVEL_COUNT

#if defined(JE_BUILD_TRACE)
#define JE_MAX_LOG_LEVEL JE_MAX_LOG_LEVEL_TRACE
#elif defined(JE_BUILD_DEBUG)
#define JE_MAX_LOG_LEVEL JE_MAX_LOG_LEVEL_DEBUG
#elif defined(JE_BUILD_DEVELOPMENT)
#define JE_MAX_LOG_LEVEL JE_MAX_LOG_LEVEL_INFO
#else
#define JE_MAX_LOG_LEVEL JE_MAX_LOG_LEVEL_NONE
#endif

#define JE_LOG_CONTEXT jeLogger_create(__FILE__, __func__, __LINE__)

#if JE_MAX_LOG_LEVEL <= JE_MAX_LOG_LEVEL_ERR
#define JE_ERROR(...) jeLogger_log(JE_LOG_CONTEXT, JE_MAX_LOG_LEVEL_ERR, __VA_ARGS__)
#define JE_WARN(...) jeLogger_log(JE_LOG_CONTEXT, JE_MAX_LOG_LEVEL_WARN, __VA_ARGS__)
#define JE_ASSERT(EXPR) jeLogger_assert(JE_LOG_CONTEXT, EXPR, #EXPR)
#define JE_DEBUGGING 1
#else
#define JE_ERROR(...)
#define JE_WARN(...)
#define JE_ASSERT(EXPR)
#define JE_DEBUGGING 0
#endif

#if JE_MAX_LOG_LEVEL <= JE_MAX_LOG_LEVEL_INFO
#define JE_INFO(...) jeLogger_log(JE_LOG_CONTEXT, JE_MAX_LOG_LEVEL_INFO, __VA_ARGS__)
#define JE_DEBUG(...) jeLogger_log(JE_LOG_CONTEXT, JE_MAX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define JE_TRACE(...) jeLogger_log(JE_LOG_CONTEXT, JE_MAX_LOG_LEVEL_TRACE, __VA_ARGS__)
#else
#define JE_INFO(...)
#define JE_DEBUG(...)
#define JE_TRACE(...)
#endif


struct jeLogger {
	const char* file;
	const char* function;
	int line;
};
JE_API_PUBLIC struct jeLogger jeLogger_create(const char* file, const char* function, int line);
JE_API_PUBLIC int jeLogger_getLevel();
JE_API_PUBLIC void jeLogger_setLevelOverride(int loggerLevelOverride);
JE_API_PUBLIC void jeLogger_log(struct jeLogger logger, int loggerLevel, const char* formatStr, ...)  JE_API_PRINTF(3, 4);
JE_API_PUBLIC void jeLogger_assert(struct jeLogger logger, bool value, const char* expressionStr);


char* JE_API_PUBLIC je_temp_buffer_allocate(int size);
char* JE_API_PUBLIC je_temp_buffer_allocate_aligned(int size, int alignment);
const char* JE_API_PUBLIC je_temp_buffer_format(const char* format_str, ...) JE_API_PRINTF(1, 2);

#endif
