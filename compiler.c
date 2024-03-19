#include <string.h>
#include "logging.h"
#include "compiler.h"

bld_compiler compiler_new(bld_compiler_type type, char* executable) {
    bld_string str = string_new();
    string_append_string(&str, executable);

    return (bld_compiler) {
        .type = type,
        .executable = string_unpack(&str),
        .flags = bld_array_new(sizeof(char*)),
    };
}

void compiler_free(bld_compiler* compiler) {
    bld_iter iter;
    char** flag;

    if (compiler == NULL) {return;}
    
    free(compiler->executable);

    iter = bld_iter_array(&compiler->flags);
    while (bld_array_next(&iter, (void**) &flag)) {
        free(*flag);
    }
    bld_array_free(&compiler->flags);
}

bld_compiler compiler_copy(bld_compiler* compiler) {
    bld_iter iter;
    bld_string str;
    char **flag, *temp;
    bld_array flags;
    bld_string executable = string_new();
    string_append_string(&executable, compiler->executable);

    flags = bld_array_copy(&compiler->flags);

    iter = bld_iter_array(&compiler->flags);
    while (bld_array_next(&iter, (void**) &flag)) {
        str = string_new();
        string_append_string(&str, *flag);
        temp = string_unpack(&str);

        *flag = temp;
    }

    return (bld_compiler) {
        .type = compiler->type,
        .executable = string_unpack(&executable),
        .flags = flags,
    };
}

uintmax_t compiler_hash(bld_compiler* compiler, uintmax_t seed) {
    bld_iter iter;
    char** flag;

    seed = string_hash(compiler->executable, seed);
    
    iter = bld_iter_array(&compiler->flags);
    while (bld_array_next(&iter, (void**) &flag)) {
        seed = string_hash(*flag, seed);
    }

    return seed;
}

void compiler_add_flag(bld_compiler* compiler, char* flag) {
    char* temp;
    bld_string s = string_new();

    string_append_string(&s, flag);
    temp = string_unpack(&s);
    bld_array_push(&compiler->flags, &temp);

    log_debug("Added flag: \"%s\"", flag);
}
