#if !defined(JE_DEBUG_H)
#define JE_DEBUG_H

#define JE_LOG_CONTEXT jeLoggerContext_create(__FILE__, __func__, __LINE__)

#define JE_LOG_LABEL_TRACE "trace"
#define JE_LOG_LABEL_DEBUG "debug"
#define JE_LOG_LABEL_LOG   "info "
#define JE_LOG_LABEL_WARN  "WARN "
#define JE_LOG_LABEL_ERR   "ERROR"

#define JE_LOG_LEVEL_TRACE 0
#define JE_LOG_LEVEL_DEBUG 1
#define JE_LOG_LEVEL_INFO  2
#define JE_LOG_LEVEL_WARN  3
#define JE_LOG_LEVEL_ERR   4
#define JE_LOG_LEVEL_NONE  5

#if defined(NDEBUG)
#define JE_LOG_LEVEL JE_LOG_LEVEL_NONE
#elif !defined(JE_BUILD_DEBUG)
#define JE_LOG_LEVEL JE_LOG_LEVEL_INFO
#elif !defined(JE_BUILD_TRACE)
#define JE_LOG_LEVEL JE_LOG_LEVEL_DEBUG
#else
#define JE_LOG_LEVEL JE_LOG_LEVEL_TRACE
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
#define JE_ERROR(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LABEL_ERR, __VA_ARGS__)
#else
#define JE_ERROR(...)
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_WARN
#define JE_WARN(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LABEL_WARN, __VA_ARGS__)
#else
#define JE_WARN(...)
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_INFO
#define JE_INFO(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LABEL_LOG, __VA_ARGS__)
#else
#define JE_INFO(...)
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_DEBUG
#define JE_DEBUG(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LABEL_DEBUG, __VA_ARGS__)
#else
#define JE_DEBUG(...)
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_TRACE
#define JE_TRACE(...) jeLogger_log(JE_LOG_CONTEXT, JE_LOG_LABEL_TRACE, __VA_ARGS__)
#else
#define JE_TRACE(...)
#endif

typedef struct jeLoggerContext jeLoggerContext;
struct jeLoggerContext {
	const char* file;
	const char* function;
	int line;
};
jeLoggerContext jeLoggerContext_create(const char* file, const char* function, int line);

void jeLogger_log(jeLoggerContext loggerContext, const char* label, const char* formatStr, ...);

#endif
