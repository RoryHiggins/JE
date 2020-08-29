#if !defined(JE_DEBUG_H)
#define JE_DEBUG_H

#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

#define JE_LOG_CONTEXT __FILE__, __LINE__
#define JE_LOG_LOG_LABEL "LOG"
#define JE_LOG_ERR_LABEL "ERR"
#if defined(NDEBUG)
#define JE_LOG jeLogger_discardLogImpl
#define JE_ERR jeLogger_discardLogImpl
#else
#define JE_LOG jeLogger_logPrefixImpl(JE_LOG_LOG_LABEL, JE_LOG_CONTEXT); jeLogger_logImpl
#define JE_ERR jeLogger_logPrefixImpl(JE_LOG_ERR_LABEL, JE_LOG_CONTEXT); jeLogger_logImpl
#endif


void jeLogger_logPrefixImpl(const char* label, const char* file, unsigned line);
void jeLogger_logImpl(const char* formatStr, ...);
void jeLogger_discardLogImpl(const char* formatStr, ...);

#endif
