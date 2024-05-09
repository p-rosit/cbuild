#ifndef SET_H
#define SET_H
#include <stdlib.h>
#include <inttypes.h>

typedef uintmax_t bld_hash;
typedef size_t bld_offset;

typedef struct bld_set {
    size_t capacity;
    size_t size;
    size_t value_size;
    size_t max_offset;
    bld_offset* offset;
    bld_hash* hash;
    void* values;
} bld_set;

bld_set     set_new(size_t);
void        set_free(bld_set*);
bld_set     set_copy(const bld_set*);
void        set_clear(bld_set*);
int         set_add(bld_set*, bld_hash, void*);
void*       set_remove(bld_set*, bld_hash);
void*       set_get(const bld_set*, bld_hash);
int         set_has(const bld_set*, bld_hash);
int         set_empty_intersection(const bld_set*, const bld_set*);

#endif
