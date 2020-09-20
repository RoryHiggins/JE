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
	JE_TRACE("buffer=%p, stride=%d", buffer, stride);

	bool ok = true;

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
void* jeBuffer_getElement(jeBuffer* buffer, int index) {
	JE_TRACE("buffer=%p, index=%d", buffer, index);

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
void* jeBuffer_get(jeBuffer* buffer) {
	return jeBuffer_getElement(buffer, 0);
}
bool jeBuffer_setCapacity(jeBuffer* buffer, int capacity) {
	JE_TRACE("buffer=%p, newCapacity=%d, currentCapacity=%d", buffer, capacity, buffer->capacity);

	bool ok = true;

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
bool jeBuffer_ensureCapacity(jeBuffer* buffer, int minCapacity) {
	JE_TRACE("buffer=%p, minCapacity=%d, currentCapacity=%d", buffer, minCapacity, buffer->capacity);

	bool ok = true;

	int newCapacity = buffer->capacity;

	static const int startCapacity = JE_BUFFER_START_CAPACITY;
	if (newCapacity < startCapacity) {
		newCapacity = startCapacity;
	}

	if (minCapacity > newCapacity) {
		newCapacity = newCapacity * 2;
	}

	if (minCapacity > newCapacity) {
		newCapacity = minCapacity;
	}

	if (newCapacity > buffer->capacity) {
		ok = ok && jeBuffer_setCapacity(buffer, newCapacity);
	}

	return ok;
}
bool jeBuffer_setCount(jeBuffer* buffer, int count) {
	JE_TRACE("buffer=%p, newCount=%d, currentCount=%d", buffer, count, buffer->count);

	bool ok = true;

	ok = ok && jeBuffer_ensureCapacity(buffer, buffer->count + count);

	if (ok) {
		buffer->count = count;
	}

	return ok;
}
bool jeBuffer_push(jeBuffer* buffer, const void* data, int count) {
	JE_TRACE("buffer=%p, count=%d", buffer, count);

	bool ok = true;

	ok = ok && jeBuffer_setCount(buffer, buffer->count + count);

	void* dest = NULL;
	if (ok) {
		dest = jeBuffer_getElement(buffer, buffer->count - count);
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

void jeString_destroy(jeString* string) {
	jeBuffer_destroy(&string->buffer);
}
bool jeString_create(jeString* string) {
	return jeBuffer_create(&string->buffer, sizeof(char));
}
bool jeString_createFormatted(jeString* string, const char* formatStr, ...) {
	JE_TRACE("string=%p, formatStr=%s", string, formatStr);

	bool ok = jeString_create(string);

	va_list args;
	va_start(args, formatStr);

	/*Copy of variadic arguments for calculating buffer size*/
	va_list argsCopy;
	va_copy(argsCopy, args);

	int formattedStringSize = -1;
	if (ok) {
		formattedStringSize = vsnprintf(/*buffer*/ NULL, 0, formatStr, argsCopy);

		JE_TRACE("string=%p, formatStr=%s, formattedStringSize=%d", string, formatStr, formattedStringSize);

		if (formattedStringSize < 0) {
			JE_ERROR("vsnprintf() failed with formatStr=%s, result=%d", formatStr, formattedStringSize);
			ok = false;
		}
	}

	ok = ok && jeString_setCount(string, formattedStringSize + 1);

	char* dest = NULL;
	if (ok) {
		dest = jeString_get(string);

		if (dest == NULL) {
			JE_ERROR("jeString_get() failed with string=%p", string);
			ok = false;
		}
	}

	ok = ok && (dest != NULL);

	if (ok) {
		formattedStringSize = vsnprintf(dest, formattedStringSize + 1, formatStr, args);

		if (formattedStringSize < 0) {
			JE_ERROR("vsnprintf() failed with dest=%p, formatStr=%s, result=%d", dest, formatStr, formattedStringSize);
			ok = false;
		}
	}

	va_end(args);

	return ok;
}
int jeBuffer_getCount(jeBuffer* buffer) {
	return buffer->count;
}
int jeBuffer_getCapacity(jeBuffer* buffer) {
	return buffer->capacity;
}
int jeString_getCount(jeString* string) {
	return string->buffer.count;
}
int jeString_getCapacity(jeString* string) {
	return string->buffer.capacity;
}
char* jeString_getElement(jeString* string, int index) {
	return (char*)jeBuffer_getElement(&string->buffer, index);
}
char* jeString_get(jeString* string) {
	return jeString_getElement(string, 0);
}
bool jeString_setCapacity(jeString* string, int capacity) {
	return jeBuffer_setCapacity(&string->buffer, capacity);
}
bool jeString_ensureCapacity(jeString* string, int minCapacity) {
	return jeBuffer_ensureCapacity(&string->buffer, minCapacity);
}
bool jeString_setCount(jeString* string, int count) {
	return jeBuffer_setCount(&string->buffer, count);
}
bool jeString_push(jeString* string, const char* data, int count) {
	return jeBuffer_push(&string->buffer, (const void*)data, count);
}
bool jeString_pushString(jeString* string, const char* data) {
	return jeString_push(string, data, strlen(data) + 1);
}

void jeContainerRunTests() {
	JE_DEBUG("");

	{
		jeBuffer buffer;
		JE_ASSERT(jeBuffer_create(&buffer, 1));
		jeBuffer_destroy(&buffer);
		JE_ASSERT(buffer.data == NULL);

		JE_ASSERT(jeBuffer_create(&buffer, sizeof(int)));
		int value = 4;
		JE_ASSERT(jeBuffer_push(&buffer, (void*)&value, 1));
		JE_ASSERT(jeBuffer_get(&buffer) != NULL);
		JE_ASSERT(jeBuffer_getCount(&buffer) == 1);
		JE_ASSERT(jeBuffer_getCapacity(&buffer) >= jeBuffer_getCount(&buffer));
		JE_ASSERT(jeBuffer_getElement(&buffer, 0) != NULL);
		JE_ASSERT(*(int*)jeBuffer_getElement(&buffer, 0) == value);
		value++;
		JE_ASSERT(*(int*)jeBuffer_getElement(&buffer, 0) != value);

		value++;
		JE_ASSERT(jeBuffer_pushOne(&buffer, (void*)&value));
		JE_ASSERT(buffer.count == 2);
		JE_ASSERT(buffer.capacity >= buffer.count);
		JE_ASSERT(*(int*)jeBuffer_getElement(&buffer, 1) == value);
		JE_ASSERT(*(int*)jeBuffer_getElement(&buffer, 0) != *(int*)jeBuffer_getElement(&buffer, 1));

		JE_ASSERT(jeBuffer_setCount(&buffer, 5));
		JE_ASSERT(buffer.count == 5);
		JE_ASSERT(buffer.capacity >= 5);

		JE_ASSERT(jeBuffer_ensureCapacity(&buffer, 10));
		JE_ASSERT(buffer.count == 5);
		JE_ASSERT(buffer.capacity >= 10);

		JE_ASSERT(jeBuffer_setCount(&buffer, 20));
		JE_ASSERT(buffer.count == 20);
		JE_ASSERT(buffer.capacity >= 20);

		JE_ASSERT(jeBuffer_getElement(&buffer, buffer.count - 1) != NULL);

		jeBuffer_destroy(&buffer);
	}

	{
		jeString string;
		JE_ASSERT(jeString_create(&string));
		JE_ASSERT(jeString_push(&string, "hello", 6));
		jeString_destroy(&string);

		JE_ASSERT(jeString_create(&string));
		JE_ASSERT(jeString_pushString(&string, "hello"));
		JE_ASSERT(jeString_get(&string) != NULL);
		JE_ASSERT(jeString_getCount(&string) >= 5);
		JE_ASSERT(jeString_getCapacity(&string) >= jeString_getCount(&string));
		JE_ASSERT(jeString_getElement(&string, 0) != NULL);

		jeString_destroy(&string);
		JE_ASSERT(jeString_createFormatted(&string, "hello %s number %d", "person", 42));
	}
}
