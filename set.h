#ifndef SET_H
#define SET_H
#include <stdlib.h>
#include <inttypes.h>

typedef uintmax_t bit_hash;
typedef size_t bit_offset;

typedef struct bit_set {
    size_t capacity;
    size_t size;
    size_t value_size;
    size_t max_offset;
    bit_offset* offset;
    bit_hash* hash;
    void* values;
} bit_set;

bit_set     set_new(size_t);
void        set_free(bit_set*);
bit_set     set_copy(const bit_set*);
void        set_clear(bit_set*);
int         set_add(bit_set*, bit_hash, void*);
void*       set_remove(bit_set*, bit_hash);
void*       set_get(const bit_set*, bit_hash);
int         set_has(const bit_set*, bit_hash);
int         set_empty_intersection(const bit_set*, const bit_set*);

#endif
