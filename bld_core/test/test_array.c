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
    int number;

    array = array_new(sizeof(int));

    number = 2;
    array_push(&array, &number);

    assert(array.capacity >= 1);
    assert(array.size == 1);
    assert(array.value_size == sizeof(int));
    assert(array.values != NULL);

    {
        int* result;

        result = array.values;
        assert(*result == number);
    }

    array_free(&array);
}

void test_array_copy(void) {
    bld_array array;
    bld_array copy_array;
    int number;

    array = array_new(sizeof(int));

    number = 3;
    array_push(&array, &number);

    copy_array = array_copy(&array);

    assert(copy_array.capacity >= array.size);
    assert(copy_array.size == array.size);
    assert(copy_array.value_size == array.value_size);
    assert(copy_array.values != NULL);

    assert(copy_array.values != array.values);
    {
        int* result;

        result = copy_array.values;
        assert(*result == number);
    }

    array_free(&array);
    array_free(&copy_array);
}

void test_array_pop(void) {
    bld_array array;
    int number[] = {2, 3};
    int* result;

    array = array_new(sizeof(int));

    array_push(&array, &number[0]);
    array_push(&array, &number[1]);
    assert(array.size == 2);

    result = array_pop(&array);
    assert(*result == number[1]);
    assert(array.size == 1);

    result = array_pop(&array);
    assert(*result == number[0]);
    assert(array.size == 0);

    array_free(&array);
}

void test_array_remove(void) {
    bld_array array;
    int number[] = {2, 3, 4};

    array = array_new(sizeof(int));

    array_push(&array, &number[0]);
    array_push(&array, &number[1]);
    array_push(&array, &number[2]);
    assert(array.size == 3);

    array_remove(&array, 1);
    assert(array.size == 2);

    {
        int* result;

        result = array.values;
        assert(result[0] == number[0]);
        assert(result[1] == number[2]);
    }

    array_free(&array);
}

void test_array_insert(void) {
    bld_array array;
    int number[] = {2, 3, 4};

    array = array_new(sizeof(int));

    array_push(&array, &number[0]);
    array_push(&array, &number[2]);
    assert(array.size == 2);

    array_insert(&array, 1, &number[1]);
    assert(array.size == 3);

    {
        int* result;

        result = array.values;
        assert(result[0] == number[0]);
        assert(result[1] == number[1]);
        assert(result[2] == number[2]);
    }

    array_free(&array);
}

void test_array_get() {
    bld_array array;
    int number;

    array = array_new(sizeof(int));

    number = 3;
    array_push(&array, &number);
    assert(array.size == 1);

    {
        int* result;

        result = array_get(&array, 0);
        assert(*result == number);
    }

    array_free(&array);
}

void test_array_reverse(void) {
    bld_array array;
    int number[] = {2, 3, 4};

    array = array_new(sizeof(int));

    array_push(&array, &number[0]);
    array_push(&array, &number[1]);
    array_push(&array, &number[2]);
    assert(array.size == 3);

    array_reverse(&array);
    assert(array.size == 3);

    {
        int* result;

        result = array.values;
        assert(result[2] == number[0]);
        assert(result[1] == number[1]);
        assert(result[0] == number[2]);
    }

    array_free(&array);
}

int main() {
    test_array_new();
    test_array_free();
    test_array_push();
    test_array_copy();
    test_array_pop();
    test_array_remove();
    test_array_insert();
    test_array_get();
    test_array_reverse();
    return 0;
}
