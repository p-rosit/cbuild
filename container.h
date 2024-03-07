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

struct bld_iter_array {
    bld_array* array;
    size_t index;
};

struct bld_iter_set {
    bld_set* set;
    size_t index;
};

typedef union bld_iter {
    struct bld_iter_array array;
    struct bld_iter_set set;
} bld_iter;

bld_array   bld_array_new(size_t);
void        bld_array_free(bld_array*);
bld_array   bld_array_copy(bld_array*);
void        bld_array_push(bld_array*, void*);
void        bld_array_pop(bld_array*, void*);

bld_set     bld_set_new(size_t);
void        bld_set_free(bld_set*);
void        bld_set_clear(bld_set*);
int         bld_set_add(bld_set*, bld_hash, void*);
int         bld_set_remove(bld_set*, bld_hash, void*);
void*       bld_set_get(bld_set*, bld_hash);
int         bld_set_has(bld_set*, bld_hash);
int         bld_set_empty_intersection(bld_set*, bld_set*);

bld_iter    bld_iter_array(bld_array*);
bld_iter    bld_iter_set(bld_set*);
int         bld_array_next(bld_iter*, void**);
int         bld_set_next(bld_iter*, void**);
#endif
