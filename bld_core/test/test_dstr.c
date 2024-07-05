#include <assert.h>
#include <string.h>
#include "../dstr.h"

void test_string_new(void) {
    bld_string str;

    str = string_new();
    assert(str.size == 0);
    assert(str.capacity > 0);
    assert(str.chars != NULL);
    assert(*str.chars == '\0');

    string_free(&str);
}

void test_string_pack(void) {
    char chars[] = "a string";
    bld_string str;

    str = string_pack(chars);
    assert(str.capacity == strlen(chars) + 1);
    assert(str.size == strlen(chars));
    assert(str.chars == chars);
}

void test_string_copy(void) {
    char chars[] = "another";
    bld_string str1;
    bld_string str2;

    str1 = string_pack(chars);
    str2 = string_copy(&str1);

    assert(str1.size == str2.size);
    assert(str1.chars != str2.chars);

    assert(str2.chars[str1.size] == '\0');
    assert(strcmp(str1.chars, str2.chars) == 0);

    string_free(&str2);
}

void test_string_unpack(void) {
    char chars[] = "string";
    bld_string str;

    str = string_pack(chars);

    {
        char* result;

        result = string_unpack(&str);
        assert(chars == result);
    }
}

void test_string_hash(void) {
    char chars[] = "string";
    bld_string str;

    str = string_pack(chars);
    string_hash(string_unpack(&str));
}

void test_string_eq(void) {
    char chars1[] = "A";
    char chars2[] = "B";
    bld_string str1;
    bld_string str2;

    {
        str1 = string_pack(chars1);
        str2 = string_pack(chars1);
        assert(string_eq(&str1, &str2));
    }

    {
        str1 = string_pack(chars1);
        str2 = string_pack(chars2);
        assert(!string_eq(&str1, &str2));
    }
}

void test_string_append_space(void) {
    bld_string str;

    str = string_new();

    string_append_space(&str);
    assert(str.capacity >= 1);
    assert(str.size == 1);
    assert(str.chars[0] == ' ');

    string_free(&str);
}

void test_string_append_char(void) {
    bld_string str;
    char c;

    str = string_new();

    c = 'b';
    string_append_char(&str, c);
    assert(str.capacity >= 1);
    assert(str.size == 1);
    assert(str.chars[0] == c);

    string_free(&str);
}

void test_string_append_string(void) {
    bld_string str;

    str = string_new();

    string_append_string(&str, "AB");
    assert(str.size == 2);
    string_append_string(&str, "CD");
    assert(str.size == 4);

    assert(strcmp(str.chars, "ABCD") == 0);

    string_free(&str);
}

int main() {
    test_string_new();
    test_string_pack();
    test_string_copy();
    test_string_unpack();
    test_string_hash();
    test_string_eq();
    test_string_append_space();
    test_string_append_char();
    return 0;
}
