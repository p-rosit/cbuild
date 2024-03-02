#ifndef CONTAINER_H
#define CONTAINER_H
#include <stdlib.h>

typedef size_t bld_hash;

typedef struct bld_array {
    size_t capacity;
    size_t size;
    void* values;
} bld_array;

typedef struct bld_set {
    size_t capacity;
    size_t size;
    size_t max_offset;
    size_t* offset;
    bld_hash* hash;
    void* values;
} bld_set;

typedef struct bld_map {
    size_t capacity;
    size_t size;
    size_t max_offset;
    size_t* offset;
    bld_hash* hash;
    void* keys;
    void* values;
} bld_map;

typedef union bld_container {
    bld_array* array;
    bld_set* set;
    bld_map* map;
} bld_container;

typedef struct bld_iter {
    bld_container as;
    size_t key_size;
    size_t value_size;
    size_t index;
} bld_iter;

bld_array   bld_array_new();
void        bld_array_free(bld_array*);
bld_array   bld_array_copy(bld_array*, size_t);
void        bld_array_push(bld_array*, void*, size_t);
void        bld_array_pop(bld_array*, void*, size_t);

bld_set     bld_set_new();
void        bld_set_free(bld_set*);
void        bld_set_add(bld_set*, bld_hash, void*, size_t);
int         bld_set_remove(bld_set*, bld_hash, void*, size_t);
int         bld_set_has(bld_set*, bld_hash, void*, size_t);

bld_map     bld_map_new();
void        bld_map_free(bld_map*);
void        bld_map_add(bld_set*, bld_hash, void*, void*, size_t, size_t);
void        bld_map_remove(bld_map*, bld_hash, void*, size_t, size_t);

bld_iter    bld_iter_array(bld_array*, size_t);
bld_iter    bld_iter_set(bld_set*, size_t);
bld_iter    bld_iter_map(bld_map*, size_t);
int         bld_array_next(bld_iter*, void**);
int         bld_set_next(bld_iter*, void**);
int         bld_map_next(bld_iter*, void**, void**);
#endif
