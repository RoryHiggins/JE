#include "stdafx.h"
#include "container.h"
#include "debug.h"


void jeBuffer_destroy(jeBuffer* buffer) {
	JE_TRACE("buffer=%p", buffer);

	free(buffer->data);

	buffer->capacity = 0;		
	buffer->count = 0;
	buffer->stride = 0;
	buffer->data = NULL;
}
bool jeBuffer_create(jeBuffer* buffer, int stride) {
	bool ok = true;

	JE_TRACE("buffer=%p, stride=%d", buffer, stride);

	if (stride <= 0) {
		JE_ERROR("invalid stride, buffer=%p, stride=%d", buffer, buffer->stride);
		ok = false;
	}

	buffer->data = NULL;
	buffer->stride = stride;
	buffer->count = 0;
	buffer->capacity = 0;

	if (!ok) {
		jeBuffer_destroy(buffer);
	}

	return ok;
}
void* jeBuffer_get(jeBuffer* buffer, int index) {
	bool ok = true;

	if (buffer->data == NULL) {
		JE_ERROR("unallocated buffer, buffer=%p, index=%d", buffer, index);
		ok = false;
	}

	if (index >= buffer->count) {
		JE_WARN("index bigger than count, buffer=%p, index=%d, count=%d", buffer, index, buffer->count);
		ok = false;
	}

	if (index >= buffer->capacity) {
		JE_ERROR("index out of bounds, buffer=%p, index=%d, capacity=%d", buffer, index, buffer->capacity);
		ok = false;
	}

	void* result = NULL;
	if (ok) {
		result = (void*)((char*)buffer->data + (buffer->stride * index));
	}

	return result;
}
bool jeBuffer_setCapacity(jeBuffer* buffer, int capacity) {
	bool ok = true;

	JE_TRACE("buffer=%p, newCapacity=%d, currentCapacity=%d", buffer, capacity, buffer->capacity);

	if (buffer->stride <= 0) {
		JE_ERROR("invalid stride, buffer=%p, stride=%d", buffer, buffer->stride);
		ok = false;
	}

	if (capacity < 0) {
		JE_ERROR("invalid capacity, buffer=%p, capacity=%d", buffer, capacity);
		ok = false;
	}

	if (ok) {
		buffer->data = realloc(buffer->data, capacity * buffer->stride);

		if (buffer->data == NULL) {
			JE_ERROR("allocation failed, buffer=%p, capacity=%d stride=%d", buffer, capacity, buffer->stride);
			jeBuffer_destroy(buffer);
			ok = false;
		}
	}

	if (ok) {
		buffer->capacity = capacity;

		if (buffer->count > buffer->capacity) {
			buffer->count = buffer->capacity;
		}
	}

	if (!ok) {
		jeBuffer_destroy(buffer);
	}

	return ok;
}
bool jeBuffer_ensureCapacity(jeBuffer* buffer, int requiredCapacity) {
	static const int startCapacity = JE_BUFFER_START_CAPACITY;

	bool ok = true;

	JE_TRACE("buffer=%p, requiredCapacity=%d, currentCapacity=%d", buffer, requiredCapacity, buffer->capacity);

	int newCapacity = buffer->capacity;

	if (newCapacity < startCapacity) {
		newCapacity = startCapacity;
	}

	if (requiredCapacity > newCapacity) {
		newCapacity = newCapacity * 2;
	}

	if (requiredCapacity > newCapacity) {
		newCapacity = requiredCapacity;
	}

	if (newCapacity > buffer->capacity) {
		ok = ok && jeBuffer_setCapacity(buffer, newCapacity);
	}

	return ok;
}
bool jeBuffer_setCount(jeBuffer* buffer, int count) {
	bool ok = true;

	JE_TRACE("buffer=%p, newCount=%d, currentCount=%d", buffer, count, buffer->count);

	ok = ok && jeBuffer_ensureCapacity(buffer, buffer->count + count);

	if (ok) {
		buffer->count = count;
	}

	return ok;
}
bool jeBuffer_push(jeBuffer* buffer, const void* data, int count) {
	bool ok = true;
	void* dest = NULL;

	JE_TRACE("buffer=%p, count=%d", buffer, count);

	ok = ok && jeBuffer_setCount(buffer, buffer->count + count);

	if (ok) {
		dest = jeBuffer_get(buffer, buffer->count - count);
	}

	ok = ok && (dest != NULL);

	if (ok) {
		memcpy(dest, data, buffer->stride * count);
	}

	return ok;
}
bool jeBuffer_pushOne(jeBuffer* buffer, const void* data) {
	JE_TRACE("buffer=%p", buffer);

	return jeBuffer_push(buffer, data, 1);
}
