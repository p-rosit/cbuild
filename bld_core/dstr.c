#include <string.h>
#include "logging.h"
#include "json.h"
#include "dstr.h"

int push_character(bld_string*, char);

bld_string string_new(void) {
    char* chars;
    bld_string str;

    chars = calloc(1, 1);
    if (chars == NULL) {log_fatal(LOG_FATAL_PREFIX "could not allocate minimal string.");}

    str.capacity = 1;
    str.size = 0;
    str.chars = chars;

    return str;
}

bld_string string_pack(char* char_ptr) {
    bld_string str;

    str.size = strlen(char_ptr);
    str.capacity = str.size + 1;
    str.chars = char_ptr;
    return str;
}

bld_string string_copy(const bld_string* str) {
    char* chars;
    bld_string cpy;

    chars = malloc(str->size + 1);
    if (chars == NULL) {
        log_fatal(LOG_FATAL_PREFIX "could not allocate copy of \"%s\".", str->chars);
    }

    memcpy(chars, str->chars, str->size);
    chars[str->size] = '\0';

    cpy.capacity = str->size + 1;
    cpy.size = str->size;
    cpy.chars = chars;

    return cpy;
}

void string_free(bld_string* str) {
    free(str->chars);
}

uintmax_t string_hash(char* str) {
    uintmax_t seed;
    char c;

    seed = 5029;
    while ((c = *str++) != '\0') {
        seed = (seed << 5) + seed + c;
    }
    return seed;
}

int push_character(bld_string* str, char c) {
    size_t capacity;
    char* chars;
    
    capacity = str->capacity;
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

int string_eq(const bld_string* str1, const bld_string* str2) {
    if (str1->size != str2->size) {return 0;}
    return strcmp(str1->chars, str2->chars) == 0;
}

void string_append_space(bld_string* str) {
    if (!push_character(str, ' ')) {
        log_fatal(LOG_FATAL_PREFIX "could not append space to string.");
    }
}

void string_append_char(bld_string* str, char c) {
    if (!push_character(str, c)) {
        log_fatal(LOG_FATAL_PREFIX "could not append \'%c\' to string.", c);
    }
}

void string_append_string(bld_string* str, char* s) {
    char *temp, c;

    temp = s;
    while ((c = *temp++) != '\0') {
        if (!push_character(str, c)) {
            log_fatal(LOG_FATAL_PREFIX "could not append \"%s\" to string.", s);
        }
    }
}

char* string_unpack(const bld_string* str) {
    return str->chars;
}

int string_parse(FILE* file, bld_string* str) {
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
