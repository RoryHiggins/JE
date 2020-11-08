#if !defined(JE_CONTAINER_H)
#define JE_CONTAINER_H

#define JE_BUFFER_START_CAPACITY 32


typedef struct jeArray jeArray;
struct jeArray {
	void* data;
	int stride;
	int count;
	int capacity;
};
void jeArray_destroy(jeArray* buffer);
bool jeArray_create(jeArray* buffer, int stride);
int jeArray_getCount(jeArray* buffer);
int jeArray_getCapacity(jeArray* buffer);
void* jeArray_getElement(jeArray* buffer, int index);
void* jeArray_get(jeArray* buffer);
bool jeArray_setCapacity(jeArray* buffer, int capacity);
bool jeArray_ensureCapacity(jeArray* buffer, int minCapacity);
bool jeArray_setCount(jeArray* buffer, int count);
bool jeArray_push(jeArray* buffer, const void* data, int count);
bool jeArray_pushOne(jeArray* buffer, const void* data);

typedef struct jeString jeString;
struct jeString {
	jeArray array;
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
