#include <string.h>
#include "logging.h"
#include "dstr.h"

int push_character(bld_string*, char);

bld_string string_new(void) {
    char* chars = malloc(1);
    if (chars == NULL) {log_fatal("Could not allocate minimal string.");}

    return (bld_string) {
        .capacity = 1,
        .size = 0,
        .chars = chars,
    };
}

bld_string string_copy(bld_string* str) {
    char* chars;

    chars = malloc(str->size + 1);
    if (chars == NULL) {
        log_fatal("Could not null terminate string \"%*s\".", (int) str->size, str->chars);
    }

    memcpy(chars, str->chars, str->size);

    return (bld_string) {
        .capacity = str->size + 1,
        .size = str->size,
        .chars = chars,
    };
}

void string_free(bld_string* str) {
    free(str->chars);
}

uintmax_t string_hash(char* str, uintmax_t seed) {
    char c;
    while ((c = *str++) != '\0') {
        seed = (seed << 5) + seed + c;
    }
    return seed;
}

int push_character(bld_string* str, char c) {
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

    str->chars[str->size++] = c;
    return 1;
}

void string_append_space(bld_string* str) {
    if (!push_character(str, ' ')) {
        log_fatal("Could not append space to string.");
    }
}

void string_append_char(bld_string* str, char c) {
    if (!push_character(str, c)) {
        log_fatal("Could not append \'%c\' to string.", c);
    }
}

void string_append_string(bld_string* str, char* s) {
    char *temp = s, c;

    while ((c = *temp++) != '\0') {
        if (!push_character(str, c)) {
            log_fatal("Could not append \"%s\" to string.", s);
        }
    }
}

char* string_unpack(bld_string* str) {
    str->chars[str->size] = '\0';
    return str->chars;
}
