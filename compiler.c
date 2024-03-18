#include <string.h>
#include "logging.h"
#include "compiler.h"

bld_compiler new_compiler(bld_compiler_type type, char* executable) {
    bld_string str = string_new();
    append_string(&str, executable);

    return (bld_compiler) {
        .type = type,
        .executable = string_unpack(&str),
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
    bld_string executable = string_new();
    append_string(&executable, compiler->executable);

    options = copy_options(&compiler->options);

    return (bld_compiler) {
        .type = compiler->type,
        .executable = string_unpack(&executable),
        .options = options,
    };
}

uintmax_t hash_compiler(bld_compiler* compiler, uintmax_t seed) {
    seed = string_hash(compiler->executable, seed);
    for (size_t i = 0; i < compiler->options.array.size; i++) {
        seed = string_hash(((char**) compiler->options.array.values)[i], seed);
    }
    return seed;
}

void add_option(bld_compiler* compiler, char* option) {
    append_option(&compiler->options, option);
    log_debug("Added option: \"%s\"", option);
}

bld_options new_options() {
    return (bld_options) {.array = bld_array_new(sizeof(char*))};
}

void free_options(bld_options* options) {
    for (size_t i = 0; i < options->array.size; i++) {
        free(((char**) options->array.values)[i]);
    }
    bld_array_free(&options->array);
}

bld_options copy_options(bld_options* options) {
    return (bld_options) {.array = bld_array_copy(&options->array)};
}

void append_option(bld_options* options, char* str) {
    char* temp;
    bld_string s = string_new();

    append_string(&s, str);
    temp = string_unpack(&s);
    bld_array_push(&options->array, &temp);
}
