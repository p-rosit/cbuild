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

void bld_array_pop(bld_array* array, void* ret_value, size_t value_size) {
    if (array->size <= 0) {log_fatal("Trying to pop from empty array");}
    memcpy(ret_value, ((char*)array->values) + --array->size * value_size, value_size);
}

bld_iter bld_iter_array(bld_array* array, size_t value_size) {
    return (bld_iter) {
        .array = (struct bld_iter_array) {
            .array = array,
            .value_size = value_size,
            .index = 0,
        }
    };
}

int bld_array_next(bld_iter* iter, void* value_ptr) {
    size_t value_size = iter->array.value_size;
    bld_array* array = iter->array.array;
    char* values = array->values;
    if (iter->array.index >= array->size) {
        return 0;
    }

    memcpy(value_ptr, values + iter->array.index++ * value_size, value_size);
    return 1;
}

bld_offset bld_hash_compute_offset(size_t capacity) {
    bld_offset max_offset = 0;

    /* Approximate log2 */
    for (bld_offset i = 0; capacity > 0; capacity /= 2, i++) {
        max_offset = (capacity % 2) ? i : max_offset;
    }

    return max_offset + (max_offset == 0 && capacity != 0);
}

size_t bld_hash_target(size_t capacity, bld_offset offset, bld_hash hash) {
    return (hash % capacity) + offset;
}

int bld_hash_find_entry(size_t capacity, bld_offset max_offset, bld_offset* offsets, bld_hash* hashes, bld_offset* offset, bld_hash* hash) {
    size_t target;
    if (capacity <= 0) {return -1;}

    *offset = 0;
    target = bld_hash_target(capacity, *offset, *hash);
    for (size_t i = target; *offset < max_offset; i++) {
        if (offsets[i] < *offset ||
            offsets[i] >= max_offset ||
            hashes[i] == *hash) {
            break;
        }

        (*offset)++;
    }

    if (*offset >= max_offset) {return -1;}

    return 0;
}

void bld_hash_swap_entry(size_t target, bld_offset* offsets, bld_hash* hashes, bld_offset* offset, bld_hash* hash) {
    bld_hash temp_hash;
    bld_offset temp_offset;

    temp_offset = offsets[target];
    temp_hash = hashes[target];

    offsets[target] = *offset;
    hashes[target] = *hash;

    *offset = temp_offset;
    *hash = temp_hash;
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

void bld_set_swap_value(bld_set* set, size_t target, bld_offset* offset, bld_hash* hash, void* value, size_t value_size) {
    void* value_ptr;
    void* temp = malloc(value_size);
    if (temp == NULL) {log_fatal("Cannot swap values");}

    bld_hash_swap_entry(target, set->offset, set->hash, offset, hash);
    value_ptr = ((char*) set->values) + target * value_size;
    memcpy(temp, value_ptr, value_size);
    memcpy(value_ptr, value, value_size);
    memcpy(value, temp, value_size);

    free(temp);
}

int bld_set_add_value(bld_set* set, size_t target, bld_offset* offset, bld_hash* hash, void* value, size_t value_size) {
    int error = -1;

    if (*offset >= set->max_offset) {return 0;}

    for (size_t i = target; i < set->capacity + set->max_offset; i++) {
        bld_set_swap_value(set, i, offset, hash, value, value_size);
        if (*offset >= set->max_offset) {error = 0; break;}

        *offset += 1;
        if (*offset >= set->max_offset) {error = -1; break;}
    }

    return error;
}

int bld_set_set_capacity(bld_set* set, size_t capacity, size_t value_size) {
    int error = 0;
    size_t i, target, total_capacity, max_offset;
    size_t* offsets;
    bld_hash* hashes;
    void* values;
    bld_set new_set;
    bld_offset offset;
    bld_hash hash;
    void* value = malloc(value_size);

    max_offset = bld_hash_compute_offset(capacity);
    total_capacity = capacity + max_offset;
    offsets = malloc(total_capacity * sizeof(bld_offset));
    hashes = malloc(total_capacity * sizeof(bld_hash));
    values = malloc(total_capacity * value_size);
    if (offsets == NULL || hashes == NULL || values == NULL || value == NULL) {
        free(offsets);
        free(hashes);
        free(values);
        free(value);
        return -1;
    }

    new_set = (bld_set) {
        .capacity = capacity,
        .size = set->size,
        .max_offset = max_offset,
        .offset = offsets,
        .hash = hashes,
        .values = values,
    };

    for (i = 0; i < new_set.capacity + new_set.max_offset; i++) {
        new_set.offset[i] = new_set.max_offset;
    }

    for (i = 0; i < set->capacity + set->max_offset; i++) {
        if (set->offset[i] >= set->max_offset) {continue;}
        offset = set->offset[i];
        hash = set->hash[i];
        memcpy(value, ((char*) set->values) + i * value_size, value_size);

        error = bld_hash_find_entry(new_set.capacity, new_set.max_offset, new_set.offset, new_set.hash, &offset, &hash);
        if (error) {break;}

        target = bld_hash_target(new_set.capacity, offset, hash);
        bld_set_swap_value(&new_set, target, &offset, &hash, value, value_size);
        if (offset >= new_set.max_offset) {continue;}

        target += 1; offset += 1;
        error = bld_set_add_value(&new_set, target, &offset, &hash, value, value_size);
        if (error) {break;};
    }

    if (!error) {
        free(set->offset);
        free(set->hash);
        free(set->values);
        *set = new_set;
    } else {
        free(new_set.offset);
        free(new_set.hash);
        free(new_set.values);
    }

    free(value);
    return error;
}

void bld_set_add(bld_set* set, bld_hash hash, void* value, size_t value_size) {
    int error;
    size_t target;
    size_t new_capacity = set->capacity;
    bld_offset offset = 0;

    if (set->capacity > 0) {
        error = bld_hash_find_entry(set->capacity, set->max_offset, set->offset, set->hash, &offset, &hash);
        target = bld_hash_target(set->capacity, offset, hash);

        if (!error && set->hash[target] == hash && set->offset[target] < set->max_offset) {
            log_warn("Trying to add value twice");
            return;
        }
    } else {
        target = 0;
        error = -1;
    }

    if (!error) {
        goto place_value;
    } else {
        goto increase_size;
    }

    do {
        increase_size:
        while (error) {
            new_capacity += new_capacity / 2 + 2 * (new_capacity < 2);
            error = bld_set_set_capacity(set, new_capacity, value_size);
        }
        error = bld_hash_find_entry(set->capacity, set->max_offset, set->offset, set->hash, &offset, &hash);
        target = bld_hash_target(set->capacity, offset, hash);
        if (error) {continue;}

        place_value:
        bld_set_swap_value(set, target, &offset, &hash, value, value_size);
        if (offset < set->max_offset) {
            target += 1; offset += 1;
            error = bld_set_add_value(set, target, &offset, &hash, value, value_size);
            if (!error) {break;}
        }
    } while (error);

    if (error) {
        log_fatal("Unable to add value to set");
    }
    set->size += 1;
}

int bld_set_has(bld_set* set, bld_hash hash) {
    size_t target;
    bld_offset offset = 0;
    int error = bld_hash_find_entry(set->capacity, set->max_offset, set->offset, set->hash, &offset, &hash);

    if (!error) {
        target = bld_hash_target(set->capacity, offset, hash);
        return set->offset[target] < set->max_offset && set->hash[target] == hash;
    }
    return 0;
}

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