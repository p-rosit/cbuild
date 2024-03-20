#include "logging.h"
#include "container.h"

bld_iter iter_array(bld_array* array) {
    return (bld_iter) {
        .type = BLD_ARRAY,
        .container = (union bld_container) {.array = array},
        .index = 0,
    };
}

int array_next(bld_iter* iter, void** value_ptr_ptr) {
    size_t value_size = iter->container.array->value_size;
    bld_array* array = iter->container.array;
    char* values = array->values;

    if (iter->index >= array->size) {
        return 0;
    }

    *value_ptr_ptr = values + iter->index++ * value_size;
    return 1;
}

bld_iter iter_set(bld_set* set) {
    return (bld_iter) {
        .type = BLD_SET,
        .container = (union bld_container) {.set = set},
        .index = 0,
    };
}

int set_next(bld_iter* iter, void** value_ptr_ptr) {
    int has_next = 0;
    size_t i = iter->index;
    size_t value_size = iter->container.set->value_size;
    bld_set* set = iter->container.set;
    char* values = set->values;
    
    while (i < set->capacity + set->max_offset) {
        if (set->offset[i] < set->max_offset) {
            has_next = 1;
            *value_ptr_ptr = values + i * value_size;
            break;
        }
        i++;
    }

    iter->index = i + 1;
    return has_next;
}

int iter_next(bld_iter* iter, void** value_ptr_ptr) {
    switch (iter->type) {
        case (BLD_ARRAY): {
            return array_next(iter, value_ptr_ptr);
        } break;
        case (BLD_SET): {
            return set_next(iter, value_ptr_ptr);
        } break;
    }
    log_fatal("iter_next: unreachable error???");
    return 0; /* Unreachable */
}
