#include <string.h>
#include "logging.h"
#include "container.h"


bld_array array_new(size_t value_size) {
    return (bld_array) {
        .capacity = 0,
        .size = 0,
        .value_size = value_size,
        .values = NULL,
    };
}

void array_free(bld_array* array) {
    free(array->values);
}

bld_array array_copy(bld_array* array) {
    bld_array copy = (bld_array) {
        .capacity = array->size,
        .size = array->size,
        .values = malloc(array->size * array->value_size),
    };
    if (copy.values == NULL) {
        log_fatal("Could not allocate");
    }

    memcpy(copy.values, array->values, array->size * array->value_size);
    return copy;
}

void array_push(bld_array* array, void* value) {
    size_t capacity = array->capacity;
    void* values;
    char* ptr;

    if (array->size >= array->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        values = realloc(array->values, capacity * array->value_size);
        if (values == NULL) {
            log_fatal("Could not increase capacity of array from %lu to %lu", array->capacity, capacity);
        }

        array->capacity = capacity;
        array->values = values;
    }

    ptr = array->values;
    memcpy(ptr + array->value_size * array->size++, value, array->value_size);
}

void* array_pop(bld_array* array) {
    if (array->size <= 0) {log_fatal("Trying to pop from empty array");}
    return ((char*) array->values) + --array->size * array->value_size;
}
