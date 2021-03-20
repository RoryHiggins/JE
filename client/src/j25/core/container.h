#pragma once

#if !defined(JE_CORE_CONTAINER_H)
#define JE_CORE_CONTAINER_H

#include <j25/core/api.h>

#include <stdbool.h>

struct jeArray {
	void* data;
	int stride;
	int count;
	int capacity;
};
JE_API_PUBLIC bool jeArray_create(struct jeArray* buffer, int stride);
JE_API_PUBLIC void jeArray_destroy(struct jeArray* buffer);
JE_API_PUBLIC int jeArray_getCount(struct jeArray* buffer);
JE_API_PUBLIC int jeArray_getCapacity(struct jeArray* buffer);
JE_API_PUBLIC void* jeArray_get(struct jeArray* buffer, int index);
JE_API_PUBLIC bool jeArray_setCapacity(struct jeArray* buffer, int capacity);
JE_API_PUBLIC bool jeArray_ensureCapacity(struct jeArray* buffer, int minCapacity);
JE_API_PUBLIC bool jeArray_setCount(struct jeArray* buffer, int count);
JE_API_PUBLIC bool jeArray_push(struct jeArray* buffer, const void* data, int count);


struct jeString {
	struct jeArray array;
};
JE_API_PUBLIC bool jeString_create(struct jeString* string);
JE_API_PUBLIC bool jeString_createFormatted(struct jeString* string, const char* formatStr, ...) JE_API_PRINTF(2, 3);
JE_API_PUBLIC void jeString_destroy(struct jeString* string);
JE_API_PUBLIC int jeString_getCount(struct jeString* string);
JE_API_PUBLIC int jeString_getCapacity(struct jeString* string);
JE_API_PUBLIC char* jeString_get(struct jeString* string, int index);
JE_API_PUBLIC bool jeString_setCapacity(struct jeString* string, int capacity);
JE_API_PUBLIC bool jeString_ensureCapacity(struct jeString* string, int minCapacity);
JE_API_PUBLIC bool jeString_setCount(struct jeString* string, int count);
JE_API_PUBLIC bool jeString_push(struct jeString* string, const char* data, int count);


JE_API_PUBLIC void jeContainer_runTests();

#endif
