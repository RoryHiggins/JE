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
void jeBuffer_create(jeBuffer* buffer, int stride);
void* jeBuffer_get(jeBuffer* buffer, int index);
void jeBuffer_setCount(jeBuffer* buffer, int count);
void jeBuffer_setCapacity(jeBuffer* buffer, int capacity);
void jeBuffer_push(jeBuffer* buffer, const void* data);

#endif
