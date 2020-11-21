#if !defined(JE_DEBUG_H)
#define JE_DEBUG_H

#include "include/common.h"

#define JE_LOG_LEVEL_TRACE 0
#define JE_LOG_LEVEL_DEBUG 1
#define JE_LOG_LEVEL_INFO  2
#define JE_LOG_LEVEL_WARN  3
#define JE_LOG_LEVEL_ERR   4
#define JE_LOG_LEVEL_COUNT 5
#define JE_LOG_LEVEL_NONE  JE_LOG_LEVEL_COUNT

#if defined(JE_BUILD_TRACE)
#define JE_LOG_LEVEL JE_LOG_LEVEL_TRACE
#elif defined(JE_BUILD_DEBUG)
#define JE_LOG_LEVEL JE_LOG_LEVEL_DEBUG
#elif defined(JE_BUILD_DEVELOPMENT)
#define JE_LOG_LEVEL JE_LOG_LEVEL_INFO
#else
#define JE_LOG_LEVEL JE_LOG_LEVEL_NONE
#endif

#define JE_LOG_CONTEXT jeLoggerContext_create(__FILE__, __func__, __LINE__)

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
#define JE_ERROR(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_ERR, __VA_ARGS__)
#define JE_WARN(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_WARN, __VA_ARGS__)
#define JE_ASSERT(EXPR) jeLogger_assert(JE_LOG_CONTEXT, EXPR, #EXPR)
#else
#define JE_ERROR(...)
#define JE_WARN(...)
#define JE_ASSERT(EXPR)
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_INFO
#define JE_INFO(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_INFO, __VA_ARGS__)
#define JE_DEBUG(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define JE_TRACE(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_TRACE, __VA_ARGS__)
#else
#define JE_INFO(...)
#define JE_DEBUG(...)
#define JE_TRACE(...)
#endif

struct jeLoggerContext {
	const char* file;
	const char* function;
	int line;
};
struct jeLoggerContext jeLoggerContext_create(const char* file, const char* function, int line);

int jeLogger_getLevel();
void jeLogger_setLevelOverride(int loggerLevelOverride);
void jeLogger_log(struct jeLoggerContext loggerContext, int loggerLevel, const char* formatStr, ...);
void jeLogger_assert(struct jeLoggerContext loggerContext, jeBool value, const char* expressionStr);

#endif
