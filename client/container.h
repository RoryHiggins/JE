#if !defined(JE_CONTAINER_H)
#define JE_CONTAINER_H

#define JE_BUFFER_START_CAPACITY 32

struct jeArray {
	void* data;
	int stride;
	int count;
	int capacity;
};
void jeArray_destroy(struct jeArray* buffer);
bool jeArray_create(struct jeArray* buffer, int stride);
int jeArray_getCount(struct jeArray* buffer);
int jeArray_getCapacity(struct jeArray* buffer);
void* jeArray_getElement(struct jeArray* buffer, int index);
void* jeArray_get(struct jeArray* buffer);
bool jeArray_setCapacity(struct jeArray* buffer, int capacity);
bool jeArray_ensureCapacity(struct jeArray* buffer, int minCapacity);
bool jeArray_setCount(struct jeArray* buffer, int count);
bool jeArray_push(struct jeArray* buffer, const void* data, int count);
bool jeArray_pushOne(struct jeArray* buffer, const void* data);

struct jeString {
	struct jeArray array;
};
void jeString_destroy(struct jeString* string);
bool jeString_create(struct jeString* string);
bool jeString_createFormatted(struct jeString* string, const char* formatStr, ...);
int jeString_getCount(struct jeString* string);
int jeString_getCapacity(struct jeString* string);
char* jeString_getElement(struct jeString* string, int index);
char* jeString_get(struct jeString* string);
bool jeString_setCapacity(struct jeString* string, int capacity);
bool jeString_ensureCapacity(struct jeString* string, int minCapacity);
bool jeString_setCount(struct jeString* string, int count);
bool jeString_push(struct jeString* string, const char* data, int count);
bool jeString_pushString(struct jeString* string, const char* data);

void jeContainerRunTests();

#endif
