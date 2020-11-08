#include "stdafx.h"
#include "container.h"
#include "debug.h"


void jeHeapArray_destroy(jeHeapArray* buffer) {
	JE_TRACE("buffer=%p", buffer);

	free(buffer->data);

	buffer->capacity = 0;
	buffer->count = 0;
	buffer->stride = 0;
	buffer->data = NULL;
}
bool jeHeapArray_create(jeHeapArray* buffer, int stride) {
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

	ok = ok && jeHeapArray_setCapacity(buffer, 0);

	if (!ok) {
		jeHeapArray_destroy(buffer);
	}

	return ok;
}
void* jeHeapArray_getElement(jeHeapArray* buffer, int index) {
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
void* jeHeapArray_get(jeHeapArray* buffer) {
	return jeHeapArray_getElement(buffer, 0);
}
bool jeHeapArray_setCapacity(jeHeapArray* buffer, int capacity) {
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

	/* ensure we have a pointer to heap memory; realloc() for size 0 may return NULL */
	int byteCapacity = capacity * buffer->stride;
	if (byteCapacity == 0) {
		byteCapacity = 1;
	}

	if (ok) {
		buffer->data = realloc(buffer->data, byteCapacity);

		if (buffer->data == NULL) {
			JE_ERROR("allocation failed, buffer=%p, capacity=%d stride=%d", buffer, capacity, buffer->stride);
			jeHeapArray_destroy(buffer);
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
		jeHeapArray_destroy(buffer);
	}

	return ok;
}
bool jeHeapArray_ensureCapacity(jeHeapArray* buffer, int minCapacity) {
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
		ok = ok && jeHeapArray_setCapacity(buffer, newCapacity);
	}

	return ok;
}
bool jeHeapArray_setCount(jeHeapArray* buffer, int count) {
	JE_TRACE("buffer=%p, newCount=%d, currentCount=%d", buffer, count, buffer->count);

	bool ok = true;

	ok = ok && jeHeapArray_ensureCapacity(buffer, buffer->count + count);

	if (ok) {
		buffer->count = count;
	}

	return ok;
}
bool jeHeapArray_push(jeHeapArray* buffer, const void* data, int count) {
	JE_TRACE("buffer=%p, count=%d", buffer, count);

	bool ok = true;

	ok = ok && jeHeapArray_setCount(buffer, buffer->count + count);

	void* dest = NULL;
	if (ok) {
		dest = jeHeapArray_getElement(buffer, buffer->count - count);
	}

	ok = ok && (dest != NULL);

	if (ok) {
		memcpy(dest, data, buffer->stride * count);
	}

	return ok;
}
bool jeHeapArray_pushOne(jeHeapArray* buffer, const void* data) {
	JE_TRACE("buffer=%p", buffer);

	return jeHeapArray_push(buffer, data, 1);
}

void jeHeapString_destroy(jeHeapString* string) {
	jeHeapArray_destroy(&string->buffer);
}
bool jeHeapString_create(jeHeapString* string) {
	return jeHeapArray_create(&string->buffer, sizeof(char));
}
bool jeHeapString_createFormatted(jeHeapString* string, const char* formatStr, ...) {
	JE_TRACE("string=%p, formatStr=%s", string, formatStr);

	bool ok = jeHeapString_create(string);

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

	ok = ok && jeHeapString_setCount(string, formattedStringSize + 1);

	char* dest = NULL;
	if (ok) {
		dest = jeHeapString_get(string);

		if (dest == NULL) {
			JE_ERROR("jeHeapString_get() failed with string=%p", string);
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
int jeHeapArray_getCount(jeHeapArray* buffer) {
	return buffer->count;
}
int jeHeapArray_getCapacity(jeHeapArray* buffer) {
	return buffer->capacity;
}
int jeHeapString_getCount(jeHeapString* string) {
	return string->buffer.count;
}
int jeHeapString_getCapacity(jeHeapString* string) {
	return string->buffer.capacity;
}
char* jeHeapString_getElement(jeHeapString* string, int index) {
	return (char*)jeHeapArray_getElement(&string->buffer, index);
}
char* jeHeapString_get(jeHeapString* string) {
	return jeHeapString_getElement(string, 0);
}
bool jeHeapString_setCapacity(jeHeapString* string, int capacity) {
	return jeHeapArray_setCapacity(&string->buffer, capacity);
}
bool jeHeapString_ensureCapacity(jeHeapString* string, int minCapacity) {
	return jeHeapArray_ensureCapacity(&string->buffer, minCapacity);
}
bool jeHeapString_setCount(jeHeapString* string, int count) {
	return jeHeapArray_setCount(&string->buffer, count);
}
bool jeHeapString_push(jeHeapString* string, const char* data, int count) {
	return jeHeapArray_push(&string->buffer, (const void*)data, count);
}
bool jeHeapString_pushString(jeHeapString* string, const char* data) {
	return jeHeapString_push(string, data, strlen(data) + 1);
}

void jeContainerRunTests() {
	JE_DEBUG("");

	{
		jeHeapArray buffer;
		JE_ASSERT(jeHeapArray_create(&buffer, 1));
		jeHeapArray_destroy(&buffer);
		JE_ASSERT(buffer.data == NULL);

		JE_ASSERT(jeHeapArray_create(&buffer, sizeof(int)));
		int value = 4;
		JE_ASSERT(jeHeapArray_push(&buffer, (void*)&value, 1));
		JE_ASSERT(jeHeapArray_get(&buffer) != NULL);
		JE_ASSERT(jeHeapArray_getCount(&buffer) == 1);
		JE_ASSERT(jeHeapArray_getCapacity(&buffer) >= jeHeapArray_getCount(&buffer));
		JE_ASSERT(jeHeapArray_getElement(&buffer, 0) != NULL);
		JE_ASSERT(*(int*)jeHeapArray_getElement(&buffer, 0) == value);
		value++;
		JE_ASSERT(*(int*)jeHeapArray_getElement(&buffer, 0) != value);

		value++;
		JE_ASSERT(jeHeapArray_pushOne(&buffer, (void*)&value));
		JE_ASSERT(buffer.count == 2);
		JE_ASSERT(buffer.capacity >= buffer.count);
		JE_ASSERT(*(int*)jeHeapArray_getElement(&buffer, 1) == value);
		JE_ASSERT(*(int*)jeHeapArray_getElement(&buffer, 0) != *(int*)jeHeapArray_getElement(&buffer, 1));

		JE_ASSERT(jeHeapArray_setCount(&buffer, 5));
		JE_ASSERT(buffer.count == 5);
		JE_ASSERT(buffer.capacity >= 5);

		JE_ASSERT(jeHeapArray_ensureCapacity(&buffer, 10));
		JE_ASSERT(buffer.count == 5);
		JE_ASSERT(buffer.capacity >= 10);

		JE_ASSERT(jeHeapArray_setCount(&buffer, 20));
		JE_ASSERT(buffer.count == 20);
		JE_ASSERT(buffer.capacity >= 20);

		JE_ASSERT(jeHeapArray_getElement(&buffer, buffer.count - 1) != NULL);

		jeHeapArray_destroy(&buffer);
	}

	{
		jeHeapString string;
		JE_ASSERT(jeHeapString_create(&string));
		JE_ASSERT(jeHeapString_push(&string, "hello", 6));
		jeHeapString_destroy(&string);

		JE_ASSERT(jeHeapString_create(&string));
		JE_ASSERT(jeHeapString_pushString(&string, "hello"));
		JE_ASSERT(jeHeapString_get(&string) != NULL);
		JE_ASSERT(jeHeapString_getCount(&string) >= 5);
		JE_ASSERT(jeHeapString_getCapacity(&string) >= jeHeapString_getCount(&string));
		JE_ASSERT(jeHeapString_getElement(&string, 0) != NULL);

		jeHeapString_destroy(&string);
		JE_ASSERT(jeHeapString_createFormatted(&string, "hello %s number %d", "person", 42));
		jeHeapString_destroy(&string);
	}
}
