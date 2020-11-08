#if !defined(JE_CONTAINER_H)
#define JE_CONTAINER_H

#include "stdafx.h"

#define JE_BUFFER_START_CAPACITY 32


typedef struct jeHeapArray jeHeapArray;
struct jeHeapArray {
	void* data;
	int stride;
	int count;
	int capacity;
};
void jeHeapArray_destroy(jeHeapArray* buffer);
bool jeHeapArray_create(jeHeapArray* buffer, int stride);
int jeHeapArray_getCount(jeHeapArray* buffer);
int jeHeapArray_getCapacity(jeHeapArray* buffer);
void* jeHeapArray_getElement(jeHeapArray* buffer, int index);
void* jeHeapArray_get(jeHeapArray* buffer);
bool jeHeapArray_setCapacity(jeHeapArray* buffer, int capacity);
bool jeHeapArray_ensureCapacity(jeHeapArray* buffer, int minCapacity);
bool jeHeapArray_setCount(jeHeapArray* buffer, int count);
bool jeHeapArray_push(jeHeapArray* buffer, const void* data, int count);
bool jeHeapArray_pushOne(jeHeapArray* buffer, const void* data);

typedef struct jeHeapString jeHeapString;
struct jeHeapString {
	jeHeapArray buffer;
};
void jeHeapString_destroy(jeHeapString* string);
bool jeHeapString_create(jeHeapString* string);
bool jeHeapString_createFormatted(jeHeapString* string, const char* formatStr, ...);
int jeHeapString_getCount(jeHeapString* string);
int jeHeapString_getCapacity(jeHeapString* string);
char* jeHeapString_getElement(jeHeapString* string, int index);
char* jeHeapString_get(jeHeapString* string);
bool jeHeapString_setCapacity(jeHeapString* string, int capacity);
bool jeHeapString_ensureCapacity(jeHeapString* string, int minCapacity);
bool jeHeapString_setCount(jeHeapString* string, int count);
bool jeHeapString_push(jeHeapString* string, const char* data, int count);
bool jeHeapString_pushString(jeHeapString* string, const char* data);

void jeContainerRunTests();

#endif
