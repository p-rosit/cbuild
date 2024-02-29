#include <string.h>
#include "logging.h"
#include "compiler.h"

bld_compiler new_compiler(bld_compiler_type type, char* executable) {
    bld_string str = new_string();
    append_string(&str, executable);

    return (bld_compiler) {
        .type = type,
        .executable = make_string(&str),
        .options = new_options(),
    };
}

void free_compiler(bld_compiler* compiler) {
    if (compiler == NULL) {return;}
    free(compiler->executable);
    free_options(&compiler->options);
}

bld_compiler copy_compiler(bld_compiler* compiler) {
    bld_options options;
    bld_string executable = new_string();
    append_string(&executable, compiler->executable);

    options = new_options();
    for (size_t i = 0; i < compiler->options.size; i++) {
        append_option(&options, compiler->options.options[i]);
    }

    return (bld_compiler) {
        .type = compiler->type,
        .executable = make_string(&executable),
        .options = options,
    };
}

uintmax_t hash_compiler(bld_compiler* compiler, uintmax_t seed) {
    seed = hash_string(compiler->executable, seed);
    for (size_t i = 0; i < compiler->options.size; i++) {
        seed = hash_string(compiler->options.options[i], seed);
    }
    return seed;
}

void add_option(bld_compiler* compiler, char* option) {
    append_option(&compiler->options, option);
    log_debug("Added option: \"%s\"", option);
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
        free(options->options);

        options->capacity = capacity;
        options->options = opts;
    }

    s = new_string();
    append_string(&s, str);
    options->options[options->size++] = make_string(&s);
}
