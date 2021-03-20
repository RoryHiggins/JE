#include <j25/core/container.h>

#include <j25/core/debug.h>

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define JE_BUFFER_START_CAPACITY 32

bool jeArray_create(struct jeArray* array, uint32_t stride) {
	JE_TRACE("array=%p, stride=%u", (void*)array, stride);

	bool ok = true;

	if (stride == 0) {
		JE_ERROR("zero stride, array=%p", (void*)array);
		ok = false;
	}

	array->data = NULL;
	array->stride = stride;
	array->count = 0;
	array->capacity = 0;

	ok = ok && jeArray_setCapacity(array, 0);

	if (!ok) {
		jeArray_destroy(array);
	}

	return ok;
}
void jeArray_destroy(struct jeArray* array) {
	JE_TRACE("array=%p", (void*)array);

	free(array->data);

	array->capacity = 0;
	array->count = 0;
	array->stride = 0;
	array->data = NULL;
}
uint32_t jeArray_getCount(struct jeArray* array) {
	return array->count;
}
uint32_t jeArray_getCapacity(struct jeArray* array) {
	return array->capacity;
}
void* jeArray_get(struct jeArray* array, uint32_t index) {
	JE_TRACE("array=%p, index=%u", (void*)array, index);

	bool ok = true;

	if (array->data == NULL) {
		JE_ERROR("unallocated array, array=%p, index=%u", (void*)array, index);
		ok = false;
	}

	if (index >= array->count) {
		JE_WARN("index bigger than count, array=%p, index=%u, count=%u", (void*)array, index, array->count);
		ok = false;
	}

	if (index >= array->capacity) {
		JE_ERROR("index out of bounds, array=%p, index=%u, capacity=%u", (void*)array, index, array->capacity);
		ok = false;
	}

	void* result = NULL;
	if (ok) {
		result = (void*)((char*)array->data + (array->stride * index));
	}

	return result;
}
bool jeArray_setCapacity(struct jeArray* array, uint32_t capacity) {
	JE_TRACE("array=%p, newCapacity=%u, currentCapacity=%u", (void*)array, capacity, array->capacity);

	bool ok = true;

	if (array->stride == 0) {
		JE_ERROR("zero stride, array=%p", (void*)array);
		ok = false;
	}

	/* ensure we have a pointer to heap memory; realloc() for size 0 may return NULL */
	uint32_t byteCapacity = capacity * array->stride;
	if (byteCapacity == 0) {
		byteCapacity = 1;
	}

	if (ok) {
		array->data = realloc(array->data, byteCapacity);

		if (array->data == NULL) {
			JE_ERROR("allocation failed, array=%p, capacity=%u stride=%u", (void*)array, capacity, array->stride);
			jeArray_destroy(array);
			ok = false;
		}
	}

	if (ok) {
		array->capacity = capacity;

		if (array->count > array->capacity) {
			array->count = array->capacity;
		}
	}

	if (!ok) {
		jeArray_destroy(array);
	}

	return ok;
}
bool jeArray_ensureCapacity(struct jeArray* array, uint32_t minCapacity) {
	JE_TRACE("array=%p, minCapacity=%u, currentCapacity=%u", (void*)array, minCapacity, array->capacity);

	bool ok = true;

	uint32_t newCapacity = array->capacity;

	if (newCapacity < JE_BUFFER_START_CAPACITY) {
		newCapacity = JE_BUFFER_START_CAPACITY;
	}

	if (minCapacity > newCapacity) {
		newCapacity = newCapacity * 2;
	}

	if (minCapacity > newCapacity) {
		newCapacity = minCapacity;
	}

	if (newCapacity > array->capacity) {
		ok = ok && jeArray_setCapacity(array, newCapacity);
	}

	return ok;
}
bool jeArray_setCount(struct jeArray* array, uint32_t count) {
	JE_TRACE("array=%p, newCount=%u, currentCount=%u", (void*)array, count, array->count);

	bool ok = true;

	ok = ok && jeArray_ensureCapacity(array, array->count + count);

	if (ok) {
		array->count = count;
	}

	return ok;
}
bool jeArray_push(struct jeArray* array, const void* data, uint32_t count) {
	JE_TRACE("array=%p, count=%u", (void*)array, count);

	bool ok = true;

	ok = ok && jeArray_setCount(array, array->count + count);

	void* dest = NULL;
	if (ok) {
		dest = jeArray_get(array, array->count - count);
	}

	ok = ok && (dest != NULL);

	if (ok) {
		memcpy(dest, data, array->stride * count);
	}

	return ok;
}

bool jeString_create(struct jeString* string) {
	return jeArray_create(&string->array, sizeof(char));
}
bool jeString_createFormatted(struct jeString* string, const char* formatStr, ...) {
	JE_TRACE("string=%p, formatStr=%s", (void*)string, formatStr);

	bool ok = jeString_create(string);

	va_list args;
	va_start(args, formatStr);

	/*Copy of variadic arguments for calculating array size*/
	va_list argsCopy;
	va_copy(argsCopy, args);

	int formattedStringSize = -1;
	if (ok) {
		formattedStringSize = vsnprintf(/*array*/ (void*)NULL, 0, formatStr, argsCopy);

		JE_TRACE("string=%p, formatStr=%s, formattedStringSize=%d", (void*)string, formatStr, formattedStringSize);

		if (formattedStringSize < 0) {
			JE_ERROR("vsnprintf() failed with formatStr=%s, formattedStringSize=%d", formatStr, formattedStringSize);
			ok = false;
		}
	}

	ok = ok && jeString_setCount(string, (uint32_t)(formattedStringSize + 1));

	char* dest = NULL;
	if (ok) {
		dest = jeString_get(string, 0);

		if (dest == NULL) {
			JE_ERROR("jeString_get() failed with string=%p", (void*)string);
			ok = false;
		}
	}

	ok = ok && (dest != NULL);

	if (ok) {
		formattedStringSize = vsnprintf(dest, (size_t)(formattedStringSize + 1), formatStr, args);

		if (formattedStringSize < 0) {
			JE_ERROR("vsnprintf() failed with dest=%p, formatStr=%s, formattedStringSize=%d", (void*)dest, formatStr, formattedStringSize);
			ok = false;
		}
	}

	va_end(args);

	return ok;
}
void jeString_destroy(struct jeString* string) {
	jeArray_destroy(&string->array);
}
uint32_t jeString_getCount(struct jeString* string) {
	return string->array.count;
}
uint32_t jeString_getCapacity(struct jeString* string) {
	return string->array.capacity;
}
char* jeString_get(struct jeString* string, uint32_t index) {
	return (char*)jeArray_get(&string->array, index);
}
bool jeString_setCapacity(struct jeString* string, uint32_t capacity) {
	return jeArray_setCapacity(&string->array, capacity);
}
bool jeString_ensureCapacity(struct jeString* string, uint32_t minCapacity) {
	return jeArray_ensureCapacity(&string->array, minCapacity);
}
bool jeString_setCount(struct jeString* string, uint32_t count) {
	return jeArray_setCount(&string->array, count);
}
bool jeString_push(struct jeString* string, const char* data, uint32_t count) {
	return jeArray_push(&string->array, (const void*)data, count);
}

void jeContainer_runTests() {
#if JE_DEBUGGING
	JE_DEBUG(" ");

	{
		struct jeArray array;
		JE_ASSERT(jeArray_create(&array, 1));
		jeArray_destroy(&array);
		JE_ASSERT(array.data == NULL);

		JE_ASSERT(jeArray_create(&array, sizeof(uint32_t)));
		uint32_t value = 4;
		JE_ASSERT(jeArray_push(&array, (void*)&value, 1));
		JE_ASSERT(jeArray_getCount(&array) == 1);
		JE_ASSERT(jeArray_getCapacity(&array) >= jeArray_getCount(&array));
		JE_ASSERT(jeArray_get(&array, 0) != NULL);
		JE_ASSERT(*(uint32_t*)jeArray_get(&array, 0) == value);
		value++;
		JE_ASSERT(*(uint32_t*)jeArray_get(&array, 0) != value);

		value++;
		JE_ASSERT(jeArray_push(&array, (void*)&value, 1));
		JE_ASSERT(array.count == 2);
		JE_ASSERT(array.capacity >= array.count);
		JE_ASSERT(*(uint32_t*)jeArray_get(&array, 1) == value);
		JE_ASSERT(*(uint32_t*)jeArray_get(&array, 0) != *(uint32_t*)jeArray_get(&array, 1));

		JE_ASSERT(jeArray_setCount(&array, 5));
		JE_ASSERT(array.count == 5);
		JE_ASSERT(array.capacity >= 5);

		JE_ASSERT(jeArray_ensureCapacity(&array, 10));
		JE_ASSERT(array.count == 5);
		JE_ASSERT(array.capacity >= 10);

		JE_ASSERT(jeArray_setCount(&array, 20));
		JE_ASSERT(array.count == 20);
		JE_ASSERT(array.capacity >= 20);

		JE_ASSERT(jeArray_get(&array, array.count - 1) != NULL);

		jeArray_destroy(&array);
	}

	{
		struct jeString string;
		JE_ASSERT(jeString_create(&string));
		JE_ASSERT(jeString_push(&string, "hello", 6));
		jeString_destroy(&string);

		JE_ASSERT(jeString_create(&string));
		JE_ASSERT(jeString_push(&string, "hello", sizeof("hello")));
		JE_ASSERT(jeString_getCount(&string) >= 5);
		JE_ASSERT(jeString_getCapacity(&string) >= jeString_getCount(&string));
		JE_ASSERT(jeString_get(&string, 0) != NULL);

		jeString_destroy(&string);
		JE_ASSERT(jeString_createFormatted(&string, "hello %s number %u", "person", 42u));
		jeString_destroy(&string);
	}
#endif
}
