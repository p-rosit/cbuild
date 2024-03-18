#include <string.h>
#include "logging.h"
#include "compiler.h"

bld_compiler new_compiler(bld_compiler_type type, char* executable) {
    bld_string str = string_new();
    string_append_string(&str, executable);

    return (bld_compiler) {
        .type = type,
        .executable = string_unpack(&str),
        .options = bld_array_new(sizeof(char*)),
    };
}

void free_compiler(bld_compiler* compiler) {
    bld_iter iter;
    char** option;

    if (compiler == NULL) {return;}
    
    free(compiler->executable);

    iter = bld_iter_array(&compiler->options);
    while (bld_array_next(&iter, (void**) &option)) {
        free(*option);
    }
    bld_array_free(&compiler->options);
}

bld_compiler copy_compiler(bld_compiler* compiler) {
    bld_iter iter;
    bld_string str;
    char **flag, *temp;
    bld_array options;
    bld_string executable = string_new();
    string_append_string(&executable, compiler->executable);

    options = bld_array_copy(&compiler->options); /* TODO: incorrect copy??? */

    iter = bld_iter_array(&compiler->options);
    while (bld_array_next(&iter, (void**) &flag)) {
        str = string_new();
        string_append_string(&str, *flag);
        temp = string_unpack(&str);

        *flag = temp;
    }

    return (bld_compiler) {
        .type = compiler->type,
        .executable = string_unpack(&executable),
        .options = options,
    };
}

uintmax_t hash_compiler(bld_compiler* compiler, uintmax_t seed) {
    bld_iter iter;
    char** option;

    seed = string_hash(compiler->executable, seed);
    
    iter = bld_iter_array(&compiler->options);
    while (bld_array_next(&iter, (void**) &option)) {
        seed = string_hash(*option, seed);
    }

    return seed;
}

void add_option(bld_compiler* compiler, char* option) {
    char* temp;
    bld_string s = string_new();

    string_append_string(&s, option);
    temp = string_unpack(&s);
    bld_array_push(&compiler->options, &temp);

    log_debug("Added option: \"%s\"", option);
}
