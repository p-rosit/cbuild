#include <string.h>
#include "logging.h"
#include "json.h"
#include "dstr.h"

int push_character(bit_string*, char);

bit_string string_new(void) {
    char* chars;
    bit_string str;

    chars = calloc(1, 1);
    if (chars == NULL) {log_fatal("Could not allocate minimal string.");}

    str.capacity = 1;
    str.size = 0;
    str.chars = chars;

    return str;
}

bit_string string_pack(char* char_ptr) {
    bit_string str;

    str.capacity = strlen(char_ptr);
    str.size = str.capacity;
    str.chars = char_ptr;
    return str;
}

bit_string string_copy(bit_string* str) {
    char* chars;
    bit_string cpy;

    chars = malloc(str->size + 1);
    if (chars == NULL) {
        log_fatal("Could not allocate copy of \"%s\".", str->chars);
    }

    memcpy(chars, str->chars, str->size);
    chars[str->size] = '\0';

    cpy.capacity = str->size + 1;
    cpy.size = str->size;
    cpy.chars = chars;

    return cpy;
}

void string_free(bit_string* str) {
    free(str->chars);
}

uintmax_t string_hash(char* str) {
    uintmax_t seed = 5029;
    char c;

    while ((c = *str++) != '\0') {
        seed = (seed << 5) + seed + c;
    }
    return seed;
}

int push_character(bit_string* str, char c) {
    size_t capacity = str->capacity;
    char* chars;
    
    if (str->capacity == 0 || str->size >= str->capacity - 1) {
        capacity += (capacity / 2) + 2 * (capacity < 2);
        chars = realloc(str->chars, capacity);
        if (chars == NULL) {
            return 0;
        }

        str->chars = chars;
        str->capacity = capacity;
    }

    str->chars[str->size] = c;
    str->chars[str->size + 1] = '\0';
    str->size += 1;
    return 1;
}

void string_append_space(bit_string* str) {
    if (!push_character(str, ' ')) {
        log_fatal("Could not append space to string.");
    }
}

void string_append_char(bit_string* str, char c) {
    if (!push_character(str, c)) {
        log_fatal("Could not append \'%c\' to string.", c);
    }
}

void string_append_string(bit_string* str, char* s) {
    char *temp = s, c;

    while ((c = *temp++) != '\0') {
        if (!push_character(str, c)) {
            log_fatal("Could not append \"%s\" to string.", s);
        }
    }
}

char* string_unpack(bit_string* str) {
    return str->chars;
}

int string_parse(FILE* file, bit_string* str) {
    int c;

    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_early_fail;}
    if (c != '\"') {log_warn("Expected string to start with \'\"\', got \'%c\'", c); goto parse_early_fail;}

    *str = string_new();
    c = getc(file);
    while (c != '\"' && c != EOF) {
        string_append_char(str, c);
        c = getc(file);
    }
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}

    return 0;
    parse_failed:
    string_free(str);
    parse_early_fail:
    return -1;
}
