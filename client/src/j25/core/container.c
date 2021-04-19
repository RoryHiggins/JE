#include <j25/core/container.h>

#include <j25/core/common.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(JE_CONTAINER_BOUNDS_CHECKING)
#define JE_CONTAINER_BOUNDS_CHECKING 1
#endif

#if !defined(JE_CONTAINER_BOUNDS_FAULT_TOLERANCE)
#define JE_CONTAINER_BOUNDS_FAULT_TOLERANCE 1
#endif

#define JE_CONTAINER_BOUNDS_FAULT_TOLERANCE_BUFFER_SIZE 1024

#define JE_BUFFER_START_CAPACITY 32

bool jeArray_create(struct jeArray* array, uint32_t stride) {
	JE_TRACE("array=%p, stride=%u", (void*)array, stride);

	bool ok = true;

	if (array == NULL) {
		JE_ERROR("array=NULL");
		ok = false;
	}

	if (stride == 0) {
		JE_ERROR("zero stride, array=%p", (void*)array);
		ok = false;
	}

	if (array != NULL) {
		memset((void*)array, 0, sizeof(struct jeArray));
	}

	if (ok) {
		array->stride = stride;
	}

	ok = ok && jeArray_setCapacity(array, 0);

	if (!ok) {
		jeArray_destroy(array);
	}

	return ok;
}
void jeArray_destroy(struct jeArray* array) {
	JE_TRACE("array=%p", (void*)array);

	if (array != NULL) {
		free(array->data);
		memset((void*)array, 0, sizeof(struct jeArray));

		array = NULL;
	}
}
uint32_t jeArray_getCount(struct jeArray* array) {
	bool ok = true;
	uint32_t count = 0;

#if JE_CONTAINER_BOUNDS_CHECKING
	if (array == NULL) {
		JE_ERROR("array=NULL");
		ok = false;
	}
#endif

	JE_MAYBE_UNUSED(ok);

	if (ok) {
		count = array->count;
	}

	return count;
}
uint32_t jeArray_getCapacity(struct jeArray* array) {
	bool ok = true;
	uint32_t capacity = 0;

#if JE_CONTAINER_BOUNDS_CHECKING
	if (array == NULL) {
		JE_ERROR("array=NULL");
		ok = false;
	}
#endif

	JE_MAYBE_UNUSED(ok);

	if (ok) {
		capacity = array->capacity;
	}

	return capacity;
}
void* jeArray_get(struct jeArray* array, uint32_t index) {
	JE_TRACE("array=%p, index=%u", (void*)array, index);

	bool ok = true;
	void* result = NULL;

#if JE_CONTAINER_BOUNDS_CHECKING
	if (array == NULL) {
		JE_ERROR("array=NULL");
		ok = false;
	}

	if (ok) {
		if (array->data == NULL) {
			JE_ERROR("unallocated array, array=%p, index=%u", (void*)array, index);
			ok = false;
		}
	}

	if (ok) {
		if (index >= array->count) {
			JE_WARN("index bigger than count, array=%p, index=%u, count=%u", (void*)array, index, array->count);
			ok = false;
		}
	}

	if (ok) {
		if (index >= array->capacity) {
			JE_ERROR("index out of bounds, array=%p, index=%u, capacity=%u", (void*)array, index, array->capacity);
			ok = false;
		}
	}
#endif

	if (ok) {
		result = (void*)((char*)array->data + (array->stride * index));
	}

#if JE_CONTAINER_BOUNDS_FAULT_TOLERANCE
	if (result == NULL) {
		static char fallbackBuffer[JE_CONTAINER_BOUNDS_FAULT_TOLERANCE_BUFFER_SIZE];
		memset((void*)fallbackBuffer, 0, JE_CONTAINER_BOUNDS_FAULT_TOLERANCE_BUFFER_SIZE);
		result = (void*)fallbackBuffer;

		JE_WARN("array out of bounds fault tolerance enabled, using fallbackBuffer=%p", (void*)fallbackBuffer);
	}
#endif

	return result;
}
bool jeArray_setCapacity(struct jeArray* array, uint32_t capacity) {
	JE_TRACE("array=%p, newCapacity=%u, currentCapacity=%u", (void*)array, capacity, array->capacity);

	bool ok = true;

	if (array == NULL) {
		JE_ERROR("array=NULL");
		ok = false;
	}

	if (ok) {
		if (array->stride == 0) {
			JE_ERROR("zero stride, array=%p", (void*)array);
			ok = false;
		}
	}

	/* ensure we have a pointer to heap memory; realloc() for size 0 may return NULL */
	if (ok) {
		uint32_t byteCapacity = capacity * array->stride;
		if (byteCapacity == 0) {
			byteCapacity = 1;
		}

		void* oldData = array->data;
		array->data = realloc(array->data, byteCapacity);
		if (array->data == NULL) {
			JE_ERROR("allocation failed, array=%p, capacity=%u stride=%u", (void*)array, capacity, array->stride);
			ok = false;

			// if realloc fails, the old pointer remains valid.
			array->data = oldData;
		}
	}

	if (ok) {
		array->capacity = capacity;

		if (array->count > array->capacity) {
			array->count = array->capacity;
		}
	}

	return ok;
}
bool jeArray_ensureCapacity(struct jeArray* array, uint32_t minCapacity) {
	JE_TRACE("array=%p, minCapacity=%u, currentCapacity=%u", (void*)array, minCapacity, array->capacity);

	bool ok = true;

	if (array == NULL) {
		JE_ERROR("array=NULL");
		ok = false;
	}

	if (ok) {
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
			ok = jeArray_setCapacity(array, newCapacity);
		}
	}

	return ok;
}
bool jeArray_setCount(struct jeArray* array, uint32_t count) {
	JE_TRACE("array=%p, newCount=%u, currentCount=%u", (void*)array, count, array->count);

	bool ok = true;

	if (array == NULL) {
		JE_ERROR("array=NULL");
		ok = false;
	}

	ok = ok && jeArray_ensureCapacity(array, count);

	if (ok) {
		array->count = count;
	}

	return ok;
}
bool jeArray_push(struct jeArray* array, const void* data, uint32_t count) {
	JE_TRACE("array=%p, count=%u", (void*)array, count);

	bool ok = true;

	if (array == NULL) {
		JE_ERROR("array=NULL");
		ok = false;
	}

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
	JE_TRACE("string=%p", (void*)string);

	bool ok = true;

	if (string == NULL) {
		JE_ERROR("string=NULL");
		ok = false;
	}

	ok = ok && jeArray_create(&string->array, sizeof(char));

	return ok;
}
void jeString_destroy(struct jeString* string) {
	if (string != NULL) {
		jeArray_destroy(&string->array);
		string = NULL;
	}
}
uint32_t jeString_getCount(struct jeString* string) {
	bool ok = true;
	uint32_t count = 0;

	if (string == NULL) {
		JE_ERROR("string=NULL");
		ok = false;
	}

	if (ok) {
		count = string->array.count;
	}

	return count;
}
uint32_t jeString_getCapacity(struct jeString* string) {
	bool ok = true;
	uint32_t capacity = 0;

	if (string == NULL) {
		JE_ERROR("string=NULL");
		ok = false;
	}

	if (ok) {
		capacity = string->array.capacity;
	}

	return capacity;
}
char* jeString_get(struct jeString* string, uint32_t index) {
	bool ok = true;
	char* result = NULL;

#if JE_CONTAINER_BOUNDS_FAULT_TOLERANCE
	result = "<jeString_get null string>";
#endif

#if JE_CONTAINER_BOUNDS_CHECKING
	if (string == NULL) {
		JE_ERROR("string=NULL");
		ok = false;
	}
#endif

	if (ok) {
		result = (char*)jeArray_get(&string->array, index);
	}

	return result;
}
bool jeString_setCapacity(struct jeString* string, uint32_t capacity) {
	bool ok = true;

	if (string == NULL) {
		JE_ERROR("string=NULL");
		ok = false;
	}

	ok = ok && jeArray_setCapacity(&string->array, capacity);
	return ok;
}
bool jeString_ensureCapacity(struct jeString* string, uint32_t minCapacity) {
	bool ok = true;

	if (string == NULL) {
		JE_ERROR("string=NULL");
		ok = false;
	}

	ok = ok && jeArray_ensureCapacity(&string->array, minCapacity);
	return ok;
}
bool jeString_setCount(struct jeString* string, uint32_t count) {
	bool ok = true;

	if (string == NULL) {
		JE_ERROR("string=NULL");
		ok = false;
	}

	ok = ok && jeArray_setCount(&string->array, count);
	return ok;
}
bool jeString_push(struct jeString* string, const char* data, uint32_t count) {
	bool ok = true;

	if (string == NULL) {
		JE_ERROR("string=NULL");
		ok = false;
	}

	if (data == NULL) {
		JE_ERROR("data=NULL");
		ok = false;
	}

	ok = ok && jeArray_push(&string->array, (const void*)data, count);
	return ok;
}
bool jeString_set(struct jeString* string, const char* data, uint32_t count) {
	bool ok = jeString_setCount(string, 0);
	ok = ok && jeString_push(string, data, count);

	return ok;
}
bool jeString_setFormatted(struct jeString* string, const char* formatStr, ...) {
	bool ok = true;

	if (string == NULL) {
		JE_ERROR("string=NULL");
		ok = false;
	}

	if (formatStr == NULL) {
		JE_ERROR("formatStr=NULL");
		ok = false;

		formatStr = "<jeString_setFormatted null formatStr>";
	}

	JE_TRACE("string=%p, formatStr=%s", (void*)string, formatStr);

	va_list args = {0};
	va_start(args, formatStr);

	/*Copy of variadic arguments for calculating array size*/
	va_list argsCopy = {0};
	va_copy(argsCopy, args);

	int formattedStringSize = -1;
	if (ok) {
		formattedStringSize = vsnprintf(/*array*/ NULL, 0, formatStr, argsCopy);

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
		formattedStringSize = vsnprintf(dest, (size_t)formattedStringSize + 1, formatStr, args);

		if (formattedStringSize < 0) {
			JE_ERROR(
				"vsnprintf() failed with dest=%p, formatStr=%s, formattedStringSize=%d",
				(void*)dest,
				formatStr,
				formattedStringSize);
			ok = false;
		}
	}

	va_end(argsCopy);
	va_end(args);

	return ok;
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
		struct jeString string = {0};
		JE_ASSERT(jeString_create(&string));
		JE_ASSERT(jeString_push(&string, "hello", 6));
		jeString_destroy(&string);

		JE_ASSERT(jeString_create(&string));
		JE_ASSERT(jeString_push(&string, "hello", sizeof("hello")));
		JE_ASSERT(jeString_getCount(&string) >= 5);
		JE_ASSERT(jeString_getCapacity(&string) >= jeString_getCount(&string));
		JE_ASSERT(jeString_get(&string, 0) != NULL);
		jeString_destroy(&string);

		JE_ASSERT(jeString_create(&string));
		const char target[] = "abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234abcd1234";
		JE_ASSERT(jeString_set(&string, target, (uint32_t)strlen(target)));
		JE_ASSERT(strcmp(jeString_get(&string, 0), target) == 0);
		jeString_destroy(&string);

		JE_ASSERT(jeString_create(&string));
		JE_ASSERT(jeString_setFormatted(&string, "hello %s number %u", "person", 42u));
		jeString_destroy(&string);
	}
#endif
}
