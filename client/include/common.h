/* Precompiled header */
#if !defined(JE_COMMON_H)
#define JE_COMMON_H

/**
 * Casts the result to void to inform the compiler that the result is not used
 * Primary use-case is to suppress unused function argument warnings
 */
#define JE_MAYBE_UNUSED(EXPR) ((void)(EXPR))

/** For future use in exposing the client calls in a shared library */
#define JE_PUBLIC

/** Boolean type for C89 support */
typedef unsigned char jeBool;

#endif
