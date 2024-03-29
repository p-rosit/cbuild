#include <string.h>
#include <stdarg.h>
#include "logging.h"
#include "compiler.h"

bld_compiler compiler_new(bld_compiler_type type, char* executable) {
    bld_compiler compiler;
    bld_string str = string_new();

    string_append_string(&str, executable);

    compiler.type = type;
    compiler.executable = string_unpack(&str);
    compiler.flags = array_new(sizeof(char*));

    return compiler;
}

bld_compiler compiler_with_flags(bld_compiler_type type, char* executable, ...) {
    bld_compiler compiler;
    va_list args;
    char* flag;

    compiler = compiler_new(type, executable);

    va_start(args, executable);
    while (1) {
        flag = va_arg(args, char*);
        if (flag == NULL) {break;}

        compiler_add_flag(&compiler, flag);
    }

    return compiler;
}

void compiler_free(bld_compiler* compiler) {
    char** flag;
    bld_iter iter;

    if (compiler == NULL) {return;}
    
    free(compiler->executable);

    iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        free(*flag);
    }
    array_free(&compiler->flags);
}

bld_compiler compiler_copy(bld_compiler* compiler) {
    bld_string str;
    char **flag, *temp;
    bld_compiler cpy;
    bld_iter iter;
    bld_array flags;
    bld_string executable = string_new();
    string_append_string(&executable, compiler->executable);

    flags = array_copy(&compiler->flags);

    iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        str = string_new();
        string_append_string(&str, *flag);
        temp = string_unpack(&str);

        *flag = temp;
    }

    cpy.type = compiler->type;
    cpy.executable = string_unpack(&executable);
    cpy.flags = flags;

    return cpy;
}

uintmax_t compiler_hash(bld_compiler* compiler, uintmax_t seed) {
    char** flag;
    bld_iter iter;

    seed = string_hash(compiler->executable, seed);
    
    iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        seed = string_hash(*flag, seed);
    }

    return seed;
}

void compiler_add_flag(bld_compiler* compiler, char* flag) {
    char* temp;
    bld_string s = string_new();

    string_append_string(&s, flag);
    temp = string_unpack(&s);
    array_push(&compiler->flags, &temp);
}
