#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if !defined(JE_CORE_API_H)
#define JE_CORE_API_H

#if !defined(JE_API_PUBLIC) && defined(JE_DLL_EXPORT) && defined(JE_DLL)
#define JE_API_PUBLIC __declspec(dllexport)
#elif !defined(JE_API_PUBLIC) && !defined(JE_DLL_EXPORT) && defined(JE_DLL)
#define JE_API_PUBLIC __declspec(dllimport)
#elif !defined(JE_API_PUBLIC)
#define JE_API_PUBLIC
#endif

#if defined(__clang__) || defined(__GNUC__)
#define JE_API_PRINTF(FORMAT_ARG, VA_ARG) __attribute__((format(printf, FORMAT_ARG, VA_ARG)))
#else
#define JE_API_PRINTF(FORMAT_ARG, VA_ARG)
#endif

#if defined(__clang__) || defined(__GNUC__)
#define JE_API_NOINLINE __attribute__ ((noinline))
#else
#define JE_API_NOINLINE
#endif

/**
 * Casts the result to void to inform the compiler that the result is not used
 * Primary use-case is to suppress unused function argument warnings
 */
#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

#define JE_LOG_LEVEL_TRACE 0
#define JE_LOG_LEVEL_DEBUG 1
#define JE_LOG_LEVEL_INFO 2
#define JE_LOG_LEVEL_WARN 3
#define JE_LOG_LEVEL_ERR 4
#define JE_LOG_LEVEL_COUNT 5
#define JE_LOG_LEVEL_NONE JE_LOG_LEVEL_COUNT

#if defined(JE_BUILD_TRACE)
#define JE_LOG_LEVEL_DEFAULT JE_LOG_LEVEL_TRACE
#define JE_LOG_LEVEL_COMPILED JE_LOG_LEVEL_TRACE
#elif defined(JE_BUILD_DEBUG)
#define JE_LOG_LEVEL_DEFAULT JE_LOG_LEVEL_DEBUG
#define JE_LOG_LEVEL_COMPILED JE_LOG_LEVEL_DEBUG
#elif defined(JE_BUILD_DEVELOPMENT)
#define JE_LOG_LEVEL_DEFAULT JE_LOG_LEVEL_INFO
#define JE_LOG_LEVEL_COMPILED JE_LOG_LEVEL_DEBUG
#else
#define JE_LOG_LEVEL_DEFAULT JE_LOG_LEVEL_NONE
#define JE_LOG_LEVEL_COMPILED JE_LOG_LEVEL_NONE
#endif

#define JE_LOG_CONTEXT jeLogger_create(__FILE__, __func__, __LINE__)

#if JE_LOG_LEVEL_DEFAULT <= JE_LOG_LEVEL_ERR
#define JE_ERROR(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_ERR, __VA_ARGS__)
#define JE_WARN(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_WARN, __VA_ARGS__)
#define JE_ASSERT(EXPR) jeLogger_assert(JE_LOG_CONTEXT, EXPR, #EXPR)
#else
#define JE_ERROR(...)
#define JE_WARN(...)
#define JE_ASSERT(EXPR)
#endif

#if JE_LOG_LEVEL_DEFAULT <= JE_LOG_LEVEL_INFO
#define JE_INFO(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_INFO, __VA_ARGS__)
#define JE_DEBUG(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define JE_DEBUGGING 1
#else
#define JE_INFO(...)
#define JE_DEBUG(...)
#define JE_DEBUGGING 0
#endif

#if JE_LOG_LEVEL_DEFAULT <= JE_LOG_LEVEL_TRACE
#define JE_TRACE(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_TRACE, __VA_ARGS__)
#else
#define JE_TRACE(...)
#endif

struct jeLogger {
	const char* file;
	const char* function;
	uint32_t line;
};

JE_API_PUBLIC struct jeLogger jeLogger_create(const char* file, const char* function, uint32_t line);
JE_API_PUBLIC uint32_t jeLogger_getLevel();
JE_API_PUBLIC void jeLogger_setLevelOverride(uint32_t loggerLevelOverride);
JE_API_PUBLIC void jeLogger_log(struct jeLogger logger, uint32_t loggerLevel, const char* formatStr, ...)
	JE_API_PRINTF(3, 4);
JE_API_PUBLIC void jeLogger_assert(struct jeLogger logger, bool value, const char* expressionStr);

JE_API_PUBLIC char* je_temp_buffer_allocate(uint32_t size);
JE_API_PUBLIC char* je_temp_buffer_allocate_aligned(uint32_t size, uint32_t alignment);
JE_API_PUBLIC const char* je_temp_buffer_format(const char* formatStr, ...) JE_API_PRINTF(1, 2);

#endif
