#include <assert.h>
#include <string.h>
#include "../path.h"

void test_path_new(void) {
    bld_path path;

    path = path_new();
    assert(path.str.capacity > 0);
    assert(path.str.size == 0);
    assert(path.str.chars != NULL);
    assert(*path.str.chars == '\0');

    path_free(&path);
}

void test_path_from_string(void) {
    bld_path path;
    char chars[] = "path";

    path = path_from_string(chars);
    assert(path.str.capacity >= strlen(chars) + 1);
    assert(path.str.size == strlen(chars));

    assert(path.str.chars != NULL);
    assert(path.str.chars[path.str.size] == '\0');
    assert(path.str.chars != chars);
    assert(strcmp(path.str.chars, chars) == 0);

    path_free(&path);
}

void test_path_append_string(void) {
    bld_path path;
    char chars_a[] = "pathA";
    char chars_b[] = "pathB";
    char result[] = "pathA" BLD_PATH_SEP "pathB";

    path = path_from_string(chars_a);
    path_append_string(&path, chars_b);

    assert(path.str.capacity >= strlen(result) + 1);
    assert(path.str.size == strlen(result));
    assert(path.str.chars != NULL);
    assert(path.str.chars[path.str.size] == '\0');
    assert(strcmp(path.str.chars, result) == 0);

    path_free(&path);
}

void test_path_append_path(void) {
    bld_path path_a;
    bld_path path_b;
    char chars_a[] = "pathA";
    char chars_b[] = "pathB";
    char result[] = "pathA" BLD_PATH_SEP "pathB";

    path_a = path_from_string(chars_a);
    path_b = path_from_string(chars_b);

    path_append_path(&path_a, &path_b);

    assert(path_a.str.capacity >= strlen(result) + 1);
    assert(path_a.str.size == strlen(result));
    assert(path_a.str.chars != NULL);
    assert(path_a.str.chars[path_a.str.size] == '\0');
    assert(strcmp(path_a.str.chars, result) == 0);

    path_free(&path_a);
    path_free(&path_b);
}

void test_path_eq(void) {
    bld_path path1;
    bld_path path2;
    char chars[] = "path" BLD_PATH_SEP "another";

    path1 = path_from_string(chars);
    path2 = path_from_string(chars);

    assert(path_eq(&path1, &path2));

    path_free(&path1);
    path_free(&path2);
}

void test_path_get_last_string(void) {
    bld_path path;
    char chars_a[] = "pathA";
    char chars_b[] = "pathB";

    path = path_from_string(chars_a);

    {
        char* result;

        result = path_get_last_string(&path);
        assert(result != NULL);
        assert(strcmp(result, chars_a) == 0);
    }

    path_append_string(&path, chars_b);

    {
        char* result;

        result = path_get_last_string(&path);
        assert(result != NULL);
        assert(strcmp(result, chars_b) == 0);
    }

    path_free(&path);
}

void test_path_remove_last_string(void) {
    bld_path path;
    char chars_a[] = "pathA";
    char chars_b[] = "pathB";
    char chars_c[] = "pathC";
    char result[] = "pathA" BLD_PATH_SEP "pathB";

    path = path_from_string(chars_a);

    {
        char* last;

        last = path_remove_last_string(&path);
        assert(last != NULL);
        assert(strcmp(last, chars_a) == 0);
        assert(path.str.size == 0);
        assert(path.str.chars[0] == '\0');
    }

    path_free(&path);
    path = path_from_string(chars_a);
    path_append_string(&path, chars_b);
    path_append_string(&path, chars_c);

    {
        char* last;

        last = path_remove_last_string(&path);
        assert(last != NULL);
        assert(strcmp(last, chars_c) == 0);

        assert(path.str.capacity >= strlen(result) + 1);
        assert(path.str.size == strlen(result));

        assert(path.str.chars != NULL);
        assert(path.str.chars[path.str.size] == '\0');
        assert(strcmp(path.str.chars, result) == 0);
    }

    path_free(&path);
}

void test_path_remove_file_ending(void) {
    bld_path path;
    char chars_a[] = "A.c";
    char chars_b[] = "A" BLD_PATH_SEP ".B" BLD_PATH_SEP "C";

    path = path_from_string(chars_a);
    {
        char result[] = "A";
        path_remove_file_ending(&path);
        assert(strcmp(path.str.chars, result) == 0);
    }
    path_free(&path);

    path = path_from_string(chars_b);
    {
        path_remove_file_ending(&path);
        assert(strcmp(path.str.chars, chars_b) == 0);
    }
    path_free(&path);
}

void test_path_ends_with(void) {
    bld_path path;
    bld_path yes_suffix;
    bld_path not_suffix;

    path = path_from_string("A" BLD_PATH_SEP "B" BLD_PATH_SEP "C");
    yes_suffix = path_from_string("B" BLD_PATH_SEP "C");
    not_suffix = path_from_string("A" BLD_PATH_SEP "B");

    assert(path_ends_with(&path, &yes_suffix));
    assert(!path_ends_with(&path, &not_suffix));

    path_free(&path);
    path_free(&yes_suffix);
    path_free(&not_suffix);
}

int main() {
    test_path_new();
    test_path_from_string();
    test_path_append_string();
    test_path_append_path();
    test_path_eq();
    test_path_get_last_string();
    test_path_remove_last_string();
    test_path_remove_file_ending();
    test_path_ends_with();
    return 0;
}
