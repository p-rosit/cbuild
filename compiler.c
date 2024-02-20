#include <string.h>
#include "compiler.h"

bld_compiler new_compiler(bld_compiler_type type, char* executable) {
    bld_string str = new_string();
    append_string(&str, executable);

    return (bld_compiler) {
        .type = type,
        .executable = str,
        .options = new_options(),
    };
}

bld_options new_options() {
    return (bld_options) {
        .capacity = 0,
        .size = 0,
        .options = NULL,
    };
}

void free_options(bld_options* options) {
    for (size_t i = 0; i < options->size; i++) {
        free(options->options[i]);
    }
    free(options->options);
}

void append_option(bld_options* options, char* str) {
    size_t capacity = options->capacity;
    char** opts;
    bld_string s;

    if (options->size >= options->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        opts = malloc(capacity * sizeof(char*));
        memcpy(opts, options->options, options->size * sizeof(char*));

        options->capacity = capacity;
        options->options = opts;
    }

    s = new_string();
    append_string(&s, str);
    options->options[options->size++] = make_string(&s);
}
