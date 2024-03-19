#ifndef CONTAINER_H
#define CONTAINER_H
#include <stdlib.h>
#include <inttypes.h>

typedef uintmax_t bld_hash;
typedef size_t bld_offset;

typedef struct bld_array {
    size_t capacity;
    size_t size;
    size_t value_size;
    void* values;
} bld_array;

typedef struct bld_set {
    size_t capacity;
    size_t size;
    size_t value_size;
    size_t max_offset;
    bld_offset* offset;
    bld_hash* hash;
    void* values;
} bld_set;

enum bld_container_type {
    BLD_ARRAY,
    BLD_SET,
};

union bld_container {
    bld_array* array;
    bld_set* set;
};

typedef struct bld_iter {
    const enum bld_container_type type;
    const union bld_container container;
    size_t index;
} bld_iter;

bld_array   array_new(size_t);
void        array_free(bld_array*);
bld_array   array_copy(bld_array*);
void        array_push(bld_array*, void*);
void        array_pop(bld_array*, void*);

bld_set     set_new(size_t);
void        set_free(bld_set*);
void        set_clear(bld_set*);
int         set_add(bld_set*, bld_hash, void*);
void*       set_get(bld_set*, bld_hash);
int         set_has(bld_set*, bld_hash);
int         set_empty_intersection(bld_set*, bld_set*);

bld_iter    iter_array(bld_array*);
bld_iter    iter_set(bld_set*);
int         iter_next(bld_iter*, void**);

#endif
