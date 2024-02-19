#include <string.h>
#include "build.h"
#include "dstr.h"

bld_string new_string() {
    return (bld_string) {
        .capacity = 0,
        .size = 0,
        .chars = NULL,
    };
}

bld_string copy_string(bld_string* str) {
    char* chars;

    chars = malloc(str->size + 1);
    if (chars == NULL) {
        log_fatal("Could not null terminate string \"%*s\".", (int) str->size, str->chars);
    }

    memcpy(chars, str->chars, str->size);

    return (bld_string) {
        .capacity = str->capacity + 1,
        .size = str->size,
        .chars = chars,
    };
}

void free_string(bld_string* str) {
    free(str->chars);
}

int push_character(bld_string* str, char c) {
    size_t capacity = str->capacity;
    char* chars;
    
    if (str->size >= str->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);
        chars = malloc(capacity);
        if (chars == NULL) {
            return -1;
        }

        memcpy(chars, str->chars, str->size);
        free(str->chars);

        str->chars = chars;
        str->capacity = capacity;
    }

    str->chars[str->size++] = c;
    return 0;
}

void append_space(bld_string* str) {
    if (!push_character(str, ' ')) {
        log_fatal("Could not append space to string.");
    }
}

void append_string(bld_string* str, char* s) {
    char *temp = s, c;

    while ((c = *temp++) != '\0') {
        if (!push_character(str, c)) {
            log_fatal("Could not append \"%s\" to string.", s);
        }
    }
}

char* make_string(bld_string* str) {
    if (!push_character(str, '\0')) {
        log_fatal("Could not null terminate string \"%*s\".", (int) str->size, str->chars);
    }
    str->size -= 1;
    return str->chars;
}
