#if !defined(JE_DEBUG_H)
#define JE_DEBUG_H

#define JE_LOG_CONTEXT jeLoggerContext_create(__FILE__, __func__, __LINE__)

#define JE_LOG_LABEL_TRACE "trace"
#define JE_LOG_LABEL_DEBUG "debug"
#define JE_LOG_LABEL_INFO  "info "
#define JE_LOG_LABEL_WARN  "WARN "
#define JE_LOG_LABEL_ERR   "ERROR"

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

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
#define JE_ERROR(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_ERR, __VA_ARGS__)
#else
#define JE_ERROR(...)
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_WARN
#define JE_WARN(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_WARN, __VA_ARGS__)
#else
#define JE_WARN(...)
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_INFO
#define JE_INFO(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_INFO, __VA_ARGS__)
#else
#define JE_INFO(...)
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_DEBUG
#define JE_DEBUG(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define JE_DEBUG(...)
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_TRACE
#define JE_TRACE(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LEVEL_TRACE, __VA_ARGS__)
#else
#define JE_TRACE(...)
#endif

#define JE_ASSERT(EXPR) jeLogger_assert(JE_LOG_CONTEXT, EXPR, #EXPR)


typedef int jeLoggerLevel;
const char* jeLoggerLevel_getLabel(jeLoggerLevel loggerLevel);
extern jeLoggerLevel jeLoggerLevel_override;

typedef struct jeLoggerContext jeLoggerContext;
struct jeLoggerContext {
	const char* file;
	const char* function;
	int line;
};
jeLoggerContext jeLoggerContext_create(const char* file, const char* function, int line);

void jeLogger_log(jeLoggerContext loggerContext, jeLoggerLevel loggerLevel, const char* formatStr, ...);
void jeLogger_assert(jeLoggerContext loggerContext, bool value, const char* expressionStr);

#endif
