#include <string.h>
#include "logging.h"
#include "container.h"

bld_offset  hash_compute_offset(size_t);
size_t      hash_target(size_t, bld_offset, bld_hash);
int         hash_find_entry(size_t, bld_offset, bld_offset*, bld_hash*, bld_offset*, bld_hash*);
void        hash_swap_entry(size_t, bld_offset*, bld_hash*, bld_offset*, bld_hash*);
void        set_swap_value(bld_set*, size_t, bld_offset*, bld_hash*, void*);
int         set_add_value(bld_set*, size_t, bld_offset*, bld_hash*, void*);
int         set_set_capacity(bld_set*, size_t);


bld_offset hash_compute_offset(size_t capacity) {
    bld_offset max_offset = 0;

    /* Approximate log2 */
    for (bld_offset i = 0; capacity > 0; capacity /= 2, i++) {
        max_offset = (capacity % 2) ? i : max_offset;
    }

    return max_offset + (max_offset == 0 && capacity != 0);
}

size_t hash_target(size_t capacity, bld_offset offset, bld_hash hash) {
    return (hash % capacity) + offset;
}

int hash_find_entry(size_t capacity, bld_offset max_offset, bld_offset* offsets, bld_hash* hashes, bld_offset* offset, bld_hash* hash) {
    size_t target;
    if (capacity <= 0) {return -1;}

    *offset = 0;
    target = hash_target(capacity, *offset, *hash);
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

void hash_swap_entry(size_t target, bld_offset* offsets, bld_hash* hashes, bld_offset* offset, bld_hash* hash) {
    bld_hash temp_hash;
    bld_offset temp_offset;

    temp_offset = offsets[target];
    temp_hash = hashes[target];

    offsets[target] = *offset;
    hashes[target] = *hash;

    *offset = temp_offset;
    *hash = temp_hash;
}

bld_set set_new(size_t value_size) {
    return (bld_set) {
        .capacity = 0,
        .size = 0,
        .value_size = value_size,
        .max_offset = 0,
        .offset = NULL,
        .hash = NULL,
        .values = NULL,
    };
}

void set_free(bld_set* set) {
    free(set->offset);
    free(set->hash);
    free(set->values);
}

void set_clear(bld_set* set) {
    for (size_t i = 0; i < set->capacity + set->max_offset; i++) {
        set->offset[i] = set->max_offset;
    }
}

void set_swap_value(bld_set* set, size_t target, bld_offset* offset, bld_hash* hash, void* value) {
    void* value_ptr;
    void* temp = malloc(set->value_size);
    if (temp == NULL) {log_fatal("Cannot swap values");}

    hash_swap_entry(target, set->offset, set->hash, offset, hash);
    value_ptr = ((char*) set->values) + target * set->value_size;
    memcpy(temp, value_ptr, set->value_size);
    memcpy(value_ptr, value, set->value_size);
    memcpy(value, temp, set->value_size);

    free(temp);
}

int set_add_value(bld_set* set, size_t target, bld_offset* offset, bld_hash* hash, void* value) {
    int error = -1;

    if (*offset >= set->max_offset) {return 0;}

    for (size_t i = target; i < set->capacity + set->max_offset; i++) {
        set_swap_value(set, i, offset, hash, value);
        if (*offset >= set->max_offset) {error = 0; break;}

        *offset += 1;
        if (*offset >= set->max_offset) {error = -1; break;}
    }

    return error;
}

int set_set_capacity(bld_set* set, size_t capacity) {
    int error = 0;
    size_t i, target, total_capacity, max_offset;
    size_t* offsets;
    bld_hash* hashes;
    void* values;
    bld_set new_set;
    bld_offset offset;
    bld_hash hash;
    void* value = malloc(set->value_size);

    max_offset = hash_compute_offset(capacity);
    total_capacity = capacity + max_offset;
    offsets = malloc(total_capacity * sizeof(bld_offset));
    hashes = malloc(total_capacity * sizeof(bld_hash));
    values = malloc(total_capacity * set->value_size);
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
        .value_size = set->value_size,
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
        memcpy(value, ((char*) set->values) + i * set->value_size, set->value_size);

        error = hash_find_entry(new_set.capacity, new_set.max_offset, new_set.offset, new_set.hash, &offset, &hash);
        if (error) {break;}

        target = hash_target(new_set.capacity, offset, hash);
        set_swap_value(&new_set, target, &offset, &hash, value);
        if (offset >= new_set.max_offset) {continue;}

        target += 1; offset += 1;
        error = set_add_value(&new_set, target, &offset, &hash, value);
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

int set_add(bld_set* set, bld_hash hash, void* value) {
    int error;
    size_t target;
    size_t new_capacity = set->capacity;
    bld_offset offset = 0;
    void* temp = malloc(set->value_size);

    if (temp == NULL) {log_fatal("Cannot add value");}
    memcpy(temp, value, set->value_size);

    if (set->capacity > 0) {
        error = hash_find_entry(set->capacity, set->max_offset, set->offset, set->hash, &offset, &hash);
        target = hash_target(set->capacity, offset, hash);

        if (!error && set->hash[target] == hash && set->offset[target] < set->max_offset) {
            log_warn("Trying to add value twice");
            free(temp);
            return -1;
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
            error = set_set_capacity(set, new_capacity);
        }
        error = hash_find_entry(set->capacity, set->max_offset, set->offset, set->hash, &offset, &hash);
        target = hash_target(set->capacity, offset, hash);
        if (error) {continue;}

        place_value:
        set_swap_value(set, target, &offset, &hash, temp);
        if (offset < set->max_offset) {
            target += 1; offset += 1;
            error = set_add_value(set, target, &offset, &hash, temp);
            if (!error) {break;}
        }
    } while (error);

    free(temp);
    if (error) {
        log_fatal("Unable to add value to set");
    }
    set->size += 1;
    return 0;
}

void* set_get(const bld_set* set, bld_hash hash) {
    int error;
    size_t offset = 0, target;
    char* values = set->values;

    if (set->capacity == 0) {return NULL;}

    error = hash_find_entry(set->capacity, set->max_offset, set->offset, set->hash, &offset, &hash);
    target = hash_target(set->capacity, offset, hash);

    if (!error && set->hash[target] == hash && set->offset[target] < set->max_offset) {
        return values + target * set->value_size;
    }

    return NULL;
}

int set_has(const bld_set* set, bld_hash hash) {
    size_t target;
    bld_offset offset = 0;
    int error;

    if (set->capacity == 0) {return 0;}

    error = hash_find_entry(set->capacity, set->max_offset, set->offset, set->hash, &offset, &hash);
    if (!error) {
        target = hash_target(set->capacity, offset, hash);
        return set->offset[target] < set->max_offset && set->hash[target] == hash;
    }
    return 0;
}

int set_empty_intersection(const bld_set* set1, const bld_set* set2) {
    for (size_t i = 0; i < set1->capacity + set1->max_offset; i++) {
        if (set1->offset[i] >= set1->max_offset) {continue;}
        if (set_has(set2, set1->hash[i])) {
            return 0;
        }
    }
    return 1;
}

bld_set set_copy(const bld_set* set) {
    bld_set cpy;
    size_t total_capacity;
    size_t* offsets;
    bld_hash* hashes;
    void* values;

    total_capacity = set->capacity + set->max_offset;
    offsets = malloc(total_capacity * sizeof(bld_offset));
    hashes = malloc(total_capacity * sizeof(bld_hash));
    values = malloc(total_capacity * set->value_size);

    if (offsets == NULL || hashes == NULL || values == NULL) {
        log_fatal("set_copy: Could not allocate space for copy");
    }

    cpy = (bld_set) {
        .capacity =  set->capacity,
        .size = set->size,
        .value_size = set->value_size,
        .max_offset = set->max_offset,
        .offset = offsets,
        .hash = hashes,
        .values = values,
    };

    memcpy(cpy.offset, set->offset, total_capacity * sizeof(bld_offset));
    memcpy(cpy.hash, set->hash, total_capacity * sizeof(bld_hash));
    memcpy(cpy.values, set->values, total_capacity * cpy.value_size);

    return cpy;
}
