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

#endif
