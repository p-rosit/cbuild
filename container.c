#include <string.h>
#include "logging.h"
#include "container.h"

bld_array bld_array_new() {
    return (bld_array) {
        .capacity = 0,
        .size = 0,
        .values = NULL,
    };
}

void bld_array_free(bld_array* array) {
    free(array->values);
}

bld_array bld_array_copy(bld_array* array, size_t value_size) {
    bld_array copy = (bld_array) {
        .capacity = array->size,
        .size = array->size,
        .values = malloc(array->size * value_size),
    };
    if (copy.values == NULL) {
        log_fatal("Could not allocate");
    }

    memcpy(copy.values, array->values, array->size * value_size);
    return copy;
}

void bld_array_push(bld_array* array, void* value, size_t value_size) {
    size_t capacity = array->capacity;
    void* values;
    char* ptr;

    if (array->size >= array->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        values = malloc(capacity * value_size);
        if (values == NULL) {
            log_fatal("Could not increase capacity of array from %lu to %lu", array->capacity, capacity);
        }

        memcpy(values, array->values, array->size * value_size);
        free(array->values);

        array->capacity = capacity;
        array->values = values;
    }

    ptr = array->values;
    memcpy(ptr + value_size * array->size++, value, value_size);
}

bld_set bld_set_new() {
    return (bld_set) {
        .capacity = 0,
        .size = 0,
        .max_offset = 0,
        .offset = NULL,
        .hash = NULL,
        .values = NULL,
    };
}

void bld_set_free(bld_set* set) {
    free(set->offset);
    free(set->hash);
    free(set->values);
}

/*
int bld_set_set_capacity(bld_set* set, size_t capacity, size_t value_size) {
    size_t* offsets = malloc(capacity * sizeof(size_t));
    bld_hash* hash = malloc(capacity * sizeof(bld_hash));
    void* values = malloc(capacity * value_size);

    if (offsets == NULL || hash == NULL || values == NULL) {
        free(offsets);
        free(hash);
        free(values);
        return -1;
    }

    for ()

    return 0;
}

void bld_set_add(bld_set* set, bld_hash hash, void* value, size_t value_size) {
    
}
*/

bld_map bld_map_new() {
    return (bld_map) {
        .capacity = 0,
        .size = 0,
        .max_offset = 0,
        .offset = NULL,
        .hash = NULL,
        .keys = NULL,
        .values = NULL,
    };
}

void bld_map_free(bld_map* map) {
    free(map->offset);
    free(map->hash);
    free(map->keys);
    free(map->values);
}

bld_iter bld_iter_array(bld_array* array, size_t value_size) {
    return (bld_iter) {
        .as = (bld_container) {.array = array},
        .key_size = 0,
        .value_size = value_size,
        .index = 0,
    };
}

int bld_array_next(bld_iter* iter, void** value_ptr) {
    if (iter->index >= iter->as.array->size) {
        return 0;
    }
    *value_ptr = ((char*) iter->as.array->values) + iter->index++ * iter->value_size;
    return 1;
}
