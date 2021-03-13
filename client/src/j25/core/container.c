#include <j25/core.h>

#define JE_BUFFER_START_CAPACITY 32

bool jeArray_create(struct jeArray* array, int stride) {
	JE_TRACE("array=%p, stride=%d", array, stride);

	bool ok = true;

	if (stride <= 0) {
		JE_ERROR("invalid stride, array=%p, stride=%d", array, array->stride);
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
	JE_TRACE("array=%p", array);

	free(array->data);

	array->capacity = 0;
	array->count = 0;
	array->stride = 0;
	array->data = NULL;
}
int jeArray_getCount(struct jeArray* array) {
	return array->count;
}
int jeArray_getCapacity(struct jeArray* array) {
	return array->capacity;
}
void* jeArray_getElement(struct jeArray* array, int index) {
	JE_TRACE("array=%p, index=%d", array, index);

	bool ok = true;

	if (array->data == NULL) {
		JE_ERROR("unallocated array, array=%p, index=%d", array, index);
		ok = false;
	}

	if (index >= array->count) {
		JE_WARN("index bigger than count, array=%p, index=%d, count=%d", array, index, array->count);
		ok = false;
	}

	if (index >= array->capacity) {
		JE_ERROR("index out of bounds, array=%p, index=%d, capacity=%d", array, index, array->capacity);
		ok = false;
	}

	void* result = NULL;
	if (ok) {
		result = (void*)((char*)array->data + (array->stride * index));
	}

	return result;
}
bool jeArray_setCapacity(struct jeArray* array, int capacity) {
	JE_TRACE("array=%p, newCapacity=%d, currentCapacity=%d", array, capacity, array->capacity);

	bool ok = true;

	if (array->stride <= 0) {
		JE_ERROR("invalid stride, array=%p, stride=%d", array, array->stride);
		ok = false;
	}

	if (capacity < 0) {
		JE_ERROR("invalid capacity, array=%p, capacity=%d", array, capacity);
		ok = false;
	}

	/* ensure we have a pointer to heap memory; realloc() for size 0 may return NULL */
	int byteCapacity = capacity * array->stride;
	if (byteCapacity == 0) {
		byteCapacity = 1;
	}

	if (ok) {
		array->data = realloc(array->data, byteCapacity);

		if (array->data == NULL) {
			JE_ERROR("allocation failed, array=%p, capacity=%d stride=%d", array, capacity, array->stride);
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
bool jeArray_ensureCapacity(struct jeArray* array, int minCapacity) {
	JE_TRACE("array=%p, minCapacity=%d, currentCapacity=%d", array, minCapacity, array->capacity);

	bool ok = true;

	int newCapacity = array->capacity;

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
bool jeArray_setCount(struct jeArray* array, int count) {
	JE_TRACE("array=%p, newCount=%d, currentCount=%d", array, count, array->count);

	bool ok = true;

	ok = ok && jeArray_ensureCapacity(array, array->count + count);

	if (ok) {
		array->count = count;
	}

	return ok;
}
bool jeArray_push(struct jeArray* array, const void* data, int count) {
	JE_TRACE("array=%p, count=%d", array, count);

	bool ok = true;

	ok = ok && jeArray_setCount(array, array->count + count);

	void* dest = NULL;
	if (ok) {
		dest = jeArray_getElement(array, array->count - count);
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
	JE_TRACE("string=%p, formatStr=%s", string, formatStr);

	bool ok = jeString_create(string);

	va_list args;
	va_start(args, formatStr);

	/*Copy of variadic arguments for calculating array size*/
	va_list argsCopy;
	va_copy(argsCopy, args);

	int formattedStringSize = -1;
	if (ok) {
		formattedStringSize = vsnprintf(/*array*/ NULL, 0, formatStr, argsCopy);

		JE_TRACE("string=%p, formatStr=%s, formattedStringSize=%d", string, formatStr, formattedStringSize);

		if (formattedStringSize < 0) {
			JE_ERROR("vsnprintf() failed with formatStr=%s, result=%d", formatStr, formattedStringSize);
			ok = false;
		}
	}

	ok = ok && jeString_setCount(string, formattedStringSize + 1);

	char* dest = NULL;
	if (ok) {
		dest = jeString_getElement(string, 0);

		if (dest == NULL) {
			JE_ERROR("jeString_getElement() failed with string=%p", string);
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
void jeString_destroy(struct jeString* string) {
	jeArray_destroy(&string->array);
}
int jeString_getCount(struct jeString* string) {
	return string->array.count;
}
int jeString_getCapacity(struct jeString* string) {
	return string->array.capacity;
}
char* jeString_getElement(struct jeString* string, int index) {
	return (char*)jeArray_getElement(&string->array, index);
}
bool jeString_setCapacity(struct jeString* string, int capacity) {
	return jeArray_setCapacity(&string->array, capacity);
}
bool jeString_ensureCapacity(struct jeString* string, int minCapacity) {
	return jeArray_ensureCapacity(&string->array, minCapacity);
}
bool jeString_setCount(struct jeString* string, int count) {
	return jeArray_setCount(&string->array, count);
}
bool jeString_push(struct jeString* string, const char* data, int count) {
	return jeArray_push(&string->array, (const void*)data, count);
}

void jeContainer_runTests() {
#if JE_DEBUGGING
	JE_DEBUG("");

	{
		struct jeArray array;
		JE_ASSERT(jeArray_create(&array, 1));
		jeArray_destroy(&array);
		JE_ASSERT(array.data == NULL);

		JE_ASSERT(jeArray_create(&array, sizeof(int)));
		int value = 4;
		JE_ASSERT(jeArray_push(&array, (void*)&value, 1));
		JE_ASSERT(jeArray_getCount(&array) == 1);
		JE_ASSERT(jeArray_getCapacity(&array) >= jeArray_getCount(&array));
		JE_ASSERT(jeArray_getElement(&array, 0) != NULL);
		JE_ASSERT(*(int*)jeArray_getElement(&array, 0) == value);
		value++;
		JE_ASSERT(*(int*)jeArray_getElement(&array, 0) != value);

		value++;
		JE_ASSERT(jeArray_push(&array, (void*)&value, 1));
		JE_ASSERT(array.count == 2);
		JE_ASSERT(array.capacity >= array.count);
		JE_ASSERT(*(int*)jeArray_getElement(&array, 1) == value);
		JE_ASSERT(*(int*)jeArray_getElement(&array, 0) != *(int*)jeArray_getElement(&array, 1));

		JE_ASSERT(jeArray_setCount(&array, 5));
		JE_ASSERT(array.count == 5);
		JE_ASSERT(array.capacity >= 5);

		JE_ASSERT(jeArray_ensureCapacity(&array, 10));
		JE_ASSERT(array.count == 5);
		JE_ASSERT(array.capacity >= 10);

		JE_ASSERT(jeArray_setCount(&array, 20));
		JE_ASSERT(array.count == 20);
		JE_ASSERT(array.capacity >= 20);

		JE_ASSERT(jeArray_getElement(&array, array.count - 1) != NULL);

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
		JE_ASSERT(jeString_getElement(&string, 0) != NULL);

		jeString_destroy(&string);
		JE_ASSERT(jeString_createFormatted(&string, "hello %s number %d", "person", 42));
		jeString_destroy(&string);
	}
#endif
}
