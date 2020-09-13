#include "stdafx.h"
#include "container.h"
#include "debug.h"

void jeBuffer_destroy(jeBuffer* buffer) {
	if (buffer->data != NULL) {
		free(buffer->data);
	}
	memset((void*)buffer, 0, sizeof(*buffer));
}
void jeBuffer_create(jeBuffer* buffer, int stride) {
	memset((void*)buffer, 0, sizeof(*buffer));
	buffer->stride = stride;

	if (buffer->stride <= 0) {
		JE_ERROR("jeBuffer_setCapacity(): invalid stride=%d", buffer->stride);
	}
}
void* jeBuffer_get(jeBuffer* buffer, int index) {
	return (void*)((char*)buffer->data + (buffer->stride * index));
}
void jeBuffer_setCapacity(jeBuffer* buffer, int capacity) {
	JE_DEBUG("jeBuffer_setCapacity(): newCapacity=%d, currentCapacity=%d", capacity, buffer->capacity);

	if (buffer->stride <= 0) {
		JE_ERROR("jeBuffer_setCapacity(): invalid stride=%d", buffer->stride);
		goto finalize;
	}

	if (capacity == buffer->capacity) {
		goto finalize;
	}

	if (capacity == 0) {
		jeBuffer_destroy(buffer);
		goto finalize;
	}

	buffer->data = realloc(buffer->data, capacity * buffer->stride);

	if (buffer->data == NULL) {
		JE_ERROR("jeBuffer_setCapacity(): allocation failed for capacity=%d stride=%d", capacity, buffer->stride);
		jeBuffer_destroy(buffer);

		goto finalize;
	}

	buffer->capacity = capacity;

	if (buffer->count > buffer->capacity) {
		buffer->count = buffer->capacity;
	}

	finalize: {
	}
}
void jeBuffer_setCount(jeBuffer* buffer, int count) {
	static const int startCapacity = JE_BUFFER_START_CAPACITY;

	int newSize = buffer->count + count;
	int newCapacity = buffer->capacity;

	if ((newSize > newCapacity) && (newCapacity < startCapacity)) {
		newCapacity = startCapacity;
	}

	if (newSize > newCapacity) {
		newCapacity = newCapacity * 2;
	}

	if (newSize > newCapacity) {
		newCapacity = newSize;
	}

	if (newCapacity > buffer->capacity) {
		jeBuffer_setCapacity(buffer, newCapacity);
	}

	buffer->count = count;
}
void jeBuffer_push(jeBuffer* buffer, const void* data) {
	jeBuffer_setCount(buffer, buffer->count + 1);

	memcpy(jeBuffer_get(buffer, buffer->count - 1), data, buffer->stride);
}
