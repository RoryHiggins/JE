#pragma once

#if !defined(JE_CORE_API_H)
#define JE_CORE_API_H

#include <stdbool.h>

#if !defined(JE_PUBLIC) && defined(JE_DLL_EXPORT) && defined(JE_DLL)
#define JE_PUBLIC __declspec(dllexport)
#elif !defined(JE_PUBLIC) && !defined(JE_DLL_EXPORT) && defined(JE_DLL)
#define JE_PUBLIC __declspec(dllimport)
#elif !defined(JE_PUBLIC)
#define JE_PUBLIC
#endif

#if defined(__clang__) || defined(__GNUC__)
#define JE_API_PRINTF(FORMAT_ARG, VA_ARG) __attribute__((format(printf, FORMAT_ARG, VA_ARG)))
#else
#define JE_API_PRINTF(FORMAT_ARG, VA_ARG)
#endif

#endif
