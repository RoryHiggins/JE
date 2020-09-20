#if !defined(JE_CONTAINER_H)
#define JE_CONTAINER_H

#include "stdafx.h"

#define JE_BUFFER_START_CAPACITY 32


typedef struct jeBuffer jeBuffer;
struct jeBuffer {
	void* data;
	int stride;
	int count;
	int capacity;
};
void jeBuffer_destroy(jeBuffer* buffer);
bool jeBuffer_create(jeBuffer* buffer, int stride);
int jeBuffer_getCount(jeBuffer* buffer);
int jeBuffer_getCapacity(jeBuffer* buffer);
void* jeBuffer_getElement(jeBuffer* buffer, int index);
void* jeBuffer_get(jeBuffer* buffer);
bool jeBuffer_setCapacity(jeBuffer* buffer, int capacity);
bool jeBuffer_ensureCapacity(jeBuffer* buffer, int minCapacity);
bool jeBuffer_setCount(jeBuffer* buffer, int count);
bool jeBuffer_push(jeBuffer* buffer, const void* data, int count);
bool jeBuffer_pushOne(jeBuffer* buffer, const void* data);

typedef struct jeString jeString;
struct jeString {
	jeBuffer buffer;
};
void jeString_destroy(jeString* string);
bool jeString_create(jeString* string);
bool jeString_createFormatted(jeString* string, const char* formatStr, ...);
int jeString_getCount(jeString* string);
int jeString_getCapacity(jeString* string);
char* jeString_getElement(jeString* string, int index);
char* jeString_get(jeString* string);
bool jeString_setCapacity(jeString* string, int capacity);
bool jeString_ensureCapacity(jeString* string, int minCapacity);
bool jeString_setCount(jeString* string, int count);
bool jeString_push(jeString* string, const char* data, int count);
bool jeString_pushString(jeString* string, const char* data);

void jeContainerRunTests();

#endif
