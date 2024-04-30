#ifndef ARRAY_H
#define ARRAY_H
#include <stdlib.h>

typedef struct bit_array {
    size_t capacity;
    size_t size;
    size_t value_size;
    void* values;
} bit_array;

bit_array   array_new(size_t);
void        array_free(bit_array*);
bit_array   array_copy(const bit_array*);
void        array_push(bit_array*, void*);
void*       array_pop(bit_array*);
void        array_insert(bit_array*, size_t, void*);
void*       array_get(const bit_array*, size_t);
void        array_reverse(bit_array*);

#endif
