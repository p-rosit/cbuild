#ifndef ARRAY_H
#define ARRAY_H
#include <stdlib.h>

typedef struct bld_array {
    size_t capacity;
    size_t size;
    size_t value_size;
    void* values;
} bld_array;

bld_array   array_new(size_t);
void        array_free(bld_array*);
bld_array   array_copy(const bld_array*);
void        array_push(bld_array*, void*);
void*       array_pop(bld_array*);
void*       array_get(bld_array*, size_t);
void        array_reverse(bld_array*);

#endif
