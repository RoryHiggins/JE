#if !defined(JE_DEBUG_H)
#define JE_DEBUG_H

#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

#define JE_LOG_CONTEXT __FILE__, __LINE__

#define JE_LOG_LABEL_DEBUG "DBG"
#define JE_LOG_LABEL_LOG "LOG"
#define JE_LOG_LABEL_ERR "ERR"

#define JE_LOG_LEVEL_DEBUG 0
#define JE_LOG_LEVEL_LOG 1
#define JE_LOG_LEVEL_ERR 2
#define JE_LOG_LEVEL_NONE 3

#if defined(NDEBUG)
#define JE_LOG_LEVEL 2
#elif !defined(JE_BUILD_DEBUG)
#define JE_LOG_LEVEL 1
#else
#define JE_LOG_LEVEL 0
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_DEBUG
#define JE_DEBUG jeLogger_logPrefixImpl(JE_LOG_LABEL_DEBUG, JE_LOG_CONTEXT); jeLogger_logImpl
#else
#define JE_DEBUG jeLogger_discardLogImpl
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_LOG
#define JE_LOG jeLogger_logPrefixImpl(JE_LOG_LABEL_LOG, JE_LOG_CONTEXT); jeLogger_logImpl
#else
#define JE_LOG jeLogger_discardLogImpl
#endif

#if JE_LOG_LEVEL <= JE_LOG_LEVEL_ERR
#define JE_ERR jeLogger_logPrefixImpl(JE_LOG_LABEL_ERR, JE_LOG_CONTEXT); jeLogger_logImpl
#else
#define JE_ERR jeLogger_discardLogImpl
#endif

void jeLogger_logPrefixImpl(const char* label, const char* file, int line);
void jeLogger_logImpl(const char* formatStr, ...);
void jeLogger_discardLogImpl(const char* formatStr, ...);

#endif
