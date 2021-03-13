#if !defined(JE_CORE_API_H)
#define JE_CORE_API_H

#include <stdbool.h>
#include <stdint.h>

/* For future use in building a DLL */
#define JE_PUBLIC

/**
 * Casts the result to void to inform the compiler that the result is not used
 * Primary use-case is to suppress unused function argument warnings
 */
#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))
 
#endif
