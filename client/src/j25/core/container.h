#pragma once

#if !defined(JE_CORE_CONTAINER_H)
#define JE_CORE_CONTAINER_H

#include <j25/core/api.h>

#include <stdbool.h>
#include <stdint.h>

struct jeArray {
	void* data;
	uint32_t stride;
	uint32_t count;
	uint32_t capacity;
};
JE_API_PUBLIC bool jeArray_create(struct jeArray* array, uint32_t stride);
JE_API_PUBLIC void jeArray_destroy(struct jeArray* array);
JE_API_PUBLIC uint32_t jeArray_getCount(struct jeArray* array);
JE_API_PUBLIC uint32_t jeArray_getCapacity(struct jeArray* array);
JE_API_PUBLIC void* jeArray_get(struct jeArray* array, uint32_t index);
JE_API_PUBLIC bool jeArray_setCapacity(struct jeArray* array, uint32_t capacity);
JE_API_PUBLIC bool jeArray_ensureCapacity(struct jeArray* array, uint32_t minCapacity);
JE_API_PUBLIC bool jeArray_setCount(struct jeArray* array, uint32_t count);
JE_API_PUBLIC bool jeArray_push(struct jeArray* array, const void* data, uint32_t count);

struct jeString {
	struct jeArray array;
};
JE_API_PUBLIC bool jeString_create(struct jeString* string);
JE_API_PUBLIC bool jeString_createFormatted(struct jeString* string, const char* formatStr, ...) JE_API_PRINTF(2, 3);
JE_API_PUBLIC void jeString_destroy(struct jeString* string);
JE_API_PUBLIC uint32_t jeString_getCount(struct jeString* string);
JE_API_PUBLIC uint32_t jeString_getCapacity(struct jeString* string);
JE_API_PUBLIC char* jeString_get(struct jeString* string, uint32_t index);
JE_API_PUBLIC bool jeString_setCapacity(struct jeString* string, uint32_t capacity);
JE_API_PUBLIC bool jeString_ensureCapacity(struct jeString* string, uint32_t minCapacity);
JE_API_PUBLIC bool jeString_setCount(struct jeString* string, uint32_t count);
JE_API_PUBLIC bool jeString_push(struct jeString* string, const char* data, uint32_t count);

JE_API_PUBLIC void jeContainer_runTests();

#endif
