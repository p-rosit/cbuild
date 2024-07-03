#include <string.h>
#include "logging.h"
#include "array.h"

int array_increase_capacity(bld_array*, size_t);


bld_array array_new(size_t value_size) {
    bld_array array;

    array.capacity = 0;
    array.size = 0;
    array.value_size = value_size;
    array.values = NULL;

    return array;
}

void array_free(bld_array* array) {
    free(array->values);
}

bld_array array_copy(const bld_array* array) {
    bld_array cpy;

    cpy.capacity = array->size;
    cpy.size = array->size;
    cpy.value_size = array->value_size;
    cpy.values = malloc(array->size * array->value_size);

    if (cpy.values == NULL) {
        log_fatal("Could not allocate");
    }

    memcpy(cpy.values, array->values, array->size * array->value_size);
    return cpy;
}

int array_increase_capacity(bld_array* array, size_t capacity) {
    void* values;

    values = realloc(array->values, capacity * array->value_size);
    if (values == NULL) {return 0;}

    array->capacity = capacity;
    array->values = values;

    return 1;
}

void array_push(bld_array* array, void* value) {
    size_t capacity;
    char* ptr;

    capacity = array->capacity;
    if (array->size >= array->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        if (!array_increase_capacity(array, capacity)) {
            log_fatal("Could not increase capacity of array from %lu to %lu", array->capacity, capacity);
        }
    }

    ptr = array->values;
    memcpy(ptr + array->value_size * array->size++, value, array->value_size);
}

void* array_pop(bld_array* array) {
    if (array->size <= 0) {log_fatal("Trying to pop from empty array");}
    return ((char*) array->values) + --array->size * array->value_size;
}

void array_remove(bld_array* array, size_t index) {
    char* ptr;
    if (array->size <= index) {log_fatal("Trying to remove value at index %lu from array of size %lu", index, array->size);}

    ptr = array->values;
    ptr += index * array->value_size;
    memmove(ptr, ptr + array->value_size, (array->size - index - 1) * array->value_size);

    array->size -= 1;
}

void array_insert(bld_array* array, size_t index, void* value) {
    size_t capacity = array->capacity;
    char* ptr;

    if (index > array->size) {
        log_fatal("Cannot insert at index %lu into array with size %lu", index, array->size);
    }

    if (array->size >= array->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        if (!array_increase_capacity(array, capacity)) {
            log_fatal("Could not increase capacity of array from %lu to %lu", array->capacity, capacity);
        }
    }

    ptr = array->values;
    ptr += index * array->value_size;

    if (index < array->size) {
        memmove(ptr + array->value_size, ptr, (array->size - index) * array->value_size);
    }

    array->size += 1;
    memcpy(ptr, value, array->value_size);
}

void* array_get(const bld_array* array, size_t index) {
    if (index >= array->size) {log_fatal("Trying to get item from index %lu but array is of size %lu", index, array->size);}
    return ((char*) array->values) + index * array->value_size;
}

void array_reverse(bld_array* array) {
    void *head, *tail, *temp;

    if (array->size <= 0) {
        return;
    }

    temp = malloc(array->value_size);
    if (temp == NULL) {log_fatal("array_reverse: internal error, could not allocate temp");}

    head = array->values;
    tail = ((char*) array->values) + (array->size - 1) * array->value_size;

    while (head < tail) {
        memcpy(temp, head, array->value_size);
        memcpy(head, tail, array->value_size);
        memcpy(tail, temp, array->value_size);

        head = ((char*) head) + array->value_size;
        tail = ((char*) tail) - array->value_size;
    }

    free(temp);
}
