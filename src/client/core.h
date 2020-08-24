#include "stdafx.h"

#if !defined(JE_LOGGING_H)
#define JE_LOGGING_H

#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))


#if !defined(__cplusplus) && (!defined(__bool_true_false_are_defined) || !__bool_true_false_are_defined)
#define true 1
#define false 0

typedef unsigned char bool;
#endif


#if defined(NDEBUG)
#define JE_LOG jeLog_discardLogImpl
#define JE_ERR jeLog_discardLogImpl
#else
#define JE_LOG jeLog_logPrefixImpl("LOG", __FILE__, __LINE__); jeLog_logImpl
#define JE_ERR jeLog_logPrefixImpl("ERR", __FILE__, __LINE__); jeLog_logImpl
#endif


void jeLog_logPrefixImpl(const char* label, const char* file, unsigned line);
void jeLog_logImpl(const char* formatStr, ...);
void jeLog_discardLogImpl(const char* formatStr, ...);

#endif
