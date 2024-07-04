#include <assert.h>
#include "../set.h"

void test_set_new(void) {
    size_t size;
    bld_set set;

    size = 11;
    set = set_new(size);

    assert(set.capacity == 0);
    assert(set.size == 0);
    assert(set.value_size == size);
    assert(set.max_offset == 0);
    assert(set.offset == NULL);
    assert(set.hash == NULL);
    assert(set.values == NULL);
}

void test_set_free(void) {
    bld_set set;

    set = set_new(11);
    set_free(&set);
}

void test_set_add(void) {
    bld_set set;
    bld_hash hash;
    int number;

    set = set_new(sizeof(int));

    hash = 0;
    number = 3;
    {
        int exists;

        exists = set_add(&set, hash, &number);
        assert(!exists);
    }

    assert(set.capacity >= 1);
    assert(set.size == 1);
    assert(set.value_size == sizeof(int));
    assert(set.max_offset > 0);
    assert(set.offset != NULL);
    assert(set.hash != NULL);
    assert(set.values != NULL);

    {
        int* stored_int;

        stored_int = set_get(&set, hash);
        assert(stored_int != NULL);
        assert(*stored_int == number);
    }

    set_free(&set);
}

void test_set_get(void) {
    bld_set set;
    bld_hash hash;
    int number;

    set = set_new(sizeof(int));

    hash = 0;
    number = 4;
    set_add(&set, hash, &number);
    assert(set.size == 1);

    {
        int* stored_int;

        stored_int = set_get(&set, hash);
        assert(stored_int != NULL);
        assert(*stored_int == number);
    }

    {
        int* missing_int;

        missing_int = set_get(&set, hash + 1);
        assert(missing_int == NULL);
    }

    set_free(&set);
}

void test_set_copy(void) {
    bld_set set;
    bld_set copy_set;
    bld_hash hash;
    int number;

    set = set_new(sizeof(int));

    hash = 0;
    number = 3;
    set_add(&set, hash, &number);

    copy_set = set_copy(&set);

    assert(copy_set.capacity >= 1);
    assert(copy_set.size == set.size);
    assert(copy_set.value_size == set.value_size);
    assert(copy_set.max_offset > 0);
    assert(copy_set.offset != NULL);
    assert(copy_set.hash != NULL);
    assert(copy_set.values != NULL);

    assert(copy_set.hash != set.hash);
    assert(copy_set.values != set.values);

    {
        int* stored_int;
        int* stored_copy;

        stored_int = set_get(&set, hash);
        stored_copy = set_get(&copy_set, hash);

        assert(stored_copy != NULL);
        assert(stored_int != stored_copy);
        assert(*stored_int == *stored_copy);
    }

    set_free(&set);
    set_free(&copy_set);
}

void test_set_clear(void) {
    size_t capacity;
    bld_set set;
    int number;

    set = set_new(sizeof(int));

    number = 4;
    set_add(&set, 0, &number);
    assert(set.size == 1);

    capacity = set.capacity;
    set_clear(&set);
    assert(set.size == 0);
    assert(set.capacity == capacity);
    assert(set.offset != NULL);
    assert(set.values != NULL);
    assert(set.hash != NULL);

    set_free(&set);
}

void test_set_remove(void) {
    bld_set set;
    bld_hash hash[] = {0, 1};
    int number[] = {3, 4};

    set = set_new(sizeof(int));

    set_add(&set, hash[0], &number[0]);
    set_add(&set, hash[1], &number[1]);
    assert(set.size == 2);

    {
        int* stored_int;
        int* missing_int;
        int* removed;

        removed = set_remove(&set, hash[0]);
        assert(removed != NULL);
        assert(*removed == number[0]);

        stored_int = set_get(&set, hash[1]);
        assert(stored_int != NULL);
        assert(*stored_int == number[1]);

        missing_int = set_get(&set, hash[0]);
        assert(missing_int == NULL);
    }

    set_free(&set);
}

void test_set_has(void) {
    bld_set set;
    bld_hash hash;
    int number;

    set = set_new(sizeof(int));

    hash = 0;
    number = 4;
    set_add(&set, hash, &number);
    assert(set.size == 1);

    assert(set_has(&set, hash));
    assert(!set_has(&set, hash + 1));

    set_free(&set);
}

void test_set_empty_intersection(void) {
    bld_set set1;
    bld_set set2;

    set1 = set_new(0);
    set2 = set_new(0);

    set_add(&set1, 0, NULL);
    set_add(&set1, 1, NULL);
    set_add(&set2, 2, NULL);
    set_add(&set2, 3, NULL);

    assert(set_empty_intersection(&set1, &set2));

    set_add(&set1, 4, NULL);
    set_add(&set2, 4, NULL);

    assert(!set_empty_intersection(&set1, &set2));

    set_free(&set1);
    set_free(&set2);
}

int main() {
    test_set_new();
    test_set_free();
    test_set_add();
    test_set_get();
    test_set_copy();
    test_set_clear();
    test_set_remove();
    test_set_has();
    test_set_empty_intersection();
    return 0;
}
