#include <assert.h>
#include "../array.h"

void test_array_new(void) {
    size_t size;
    bld_array array;

    size = 10;
    array = array_new(size);

    assert(array.capacity == 0);
    assert(array.size == 0);
    assert(array.value_size == size);
    assert(array.values == NULL);
}

void test_array_free(void) {
    bld_array array;

    array = array_new(10);
    array_free(&array);
}

void test_array_push(void) {
    bld_array array;
    int num;

    array = array_new(sizeof(int));

    num = 2;
    array_push(&array, &num);

    assert(array.capacity >= 1);
    assert(array.size == 1);
    assert(array.value_size == sizeof(int));
    assert(array.values != NULL);

    {
        int* result;

        result = array.values;
        assert(*result == num);
    }

    array_free(&array);
}

int main() {
    test_array_new();
    test_array_free();
    test_array_push();
    return 0;
}
