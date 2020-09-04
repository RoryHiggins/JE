#if !defined(JE_DEBUG_H)
#define JE_DEBUG_H

#define JE_LOG_CONTEXT __FILE__, __LINE__

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
#define JE_LOG_LEVEL JE_LOG_LEVEL_ERR
#elif !defined(JE_BUILD_DEBUG)
#define JE_LOG_LEVEL JE_LOG_LEVEL_INFO
#elif !defined(JE_BUILD_TRACE)
#define JE_LOG_LEVEL JE_LOG_LEVEL_DEBUG
#else
#define JE_LOG_LEVEL JE_LOG_LEVEL_TRACE
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
#define JE_ERROR jeLogger_logPrefixImpl(JE_LOG_LABEL_ERR, JE_LOG_CONTEXT); jeLogger_logImpl
#else
#define JE_ERROR jeLogger_discardLogImpl
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_WARN
#define JE_WARN jeLogger_logPrefixImpl(JE_LOG_LABEL_WARN, JE_LOG_CONTEXT); jeLogger_logImpl
#else
#define JE_WARN jeLogger_discardLogImpl
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_INFO
#define JE_INFO jeLogger_logPrefixImpl(JE_LOG_LABEL_LOG, JE_LOG_CONTEXT); jeLogger_logImpl
#else
#define JE_INFO jeLogger_discardLogImpl
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_DEBUG
#define JE_DEBUG jeLogger_logPrefixImpl(JE_LOG_LABEL_DEBUG, JE_LOG_CONTEXT); jeLogger_logImpl
#else
#define JE_DEBUG jeLogger_discardLogImpl
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_TRACE
#define JE_TRACE jeLogger_logPrefixImpl(JE_LOG_LABEL_TRACE, JE_LOG_CONTEXT); jeLogger_logImpl
#else
#define JE_TRACE jeLogger_discardLogImpl
#endif


void jeLogger_logPrefixImpl(const char* label, const char* file, int line);
void jeLogger_logImpl(const char* formatStr, ...);
void jeLogger_discardLogImpl(const char* formatStr, ...);

#endif
