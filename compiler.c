#include <string.h>
#include <stdarg.h>
#include "logging.h"
#include "json.h"
#include "compiler.h"

bld_compiler compiler_new(char* executable) {
    bld_compiler compiler;
    bld_string str;

    str = string_pack(executable);
    compiler.executable = string_copy(&str);
    compiler.flags = compiler_flags_new();

    return compiler;
}

bld_compiler compiler_with_flags(char* executable, ...) {
    bld_compiler compiler;
    va_list args;
    char* flag;

    compiler = compiler_new(executable);

    va_start(args, executable);
    while (1) {
        flag = va_arg(args, char*);
        if (flag == NULL) {break;}

        compiler_add_flag(&compiler, flag);
    }

    return compiler;
}

void compiler_free(bld_compiler* compiler) {
    if (compiler == NULL) {return;}
    
    string_free(&compiler->executable);
    compiler_flags_free(&compiler->flags);
}

bld_compiler compiler_copy(bld_compiler* compiler) {
    bld_compiler cpy;

    cpy.executable = string_copy(&compiler->executable);
    cpy.flags = compiler_flags_copy(&compiler->flags);

    return cpy;
}

uintmax_t compiler_hash(bld_compiler* compiler) {
    uintmax_t seed = 2349;

    seed = (seed << 5) + string_hash(string_unpack(&compiler->executable));
    seed = (seed << 5) + compiler_flags_hash(&compiler->flags);

    return seed;
}

void compiler_add_flag(bld_compiler* compiler, char* flag) {
    compiler_flags_add_flag(&compiler->flags, flag);
}

void compiler_remove_flag(bld_compiler* compiler, char* flag) {
    compiler_flags_remove_flag(&compiler->flags, flag);
}

bld_compiler_flags compiler_flags_new(void) {
    bld_compiler_flags flags;
    flags.flags = array_new(sizeof(bld_string));
    flags.flag_hash = set_new(0);
    flags.removed = set_new(sizeof(bld_string));
    return flags;
}

void compiler_flags_free(bld_compiler_flags* flags) {
    bld_iter iter;
    bld_string* flag;

    iter = iter_array(&flags->flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(flag);
    }
    array_free(&flags->flags);
    set_free(&flags->flag_hash);

    iter = iter_set(&flags->removed);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(flag);
    }
    set_free(&flags->removed);
}

bld_compiler_flags compiler_flags_with_flags(char* first_flag, ...) {
    bld_compiler_flags flags;
    va_list args;
    char* flag;

    flags = compiler_flags_new();

    if (first_flag == NULL) {
        return flags;
    }
    compiler_flags_add_flag(&flags, first_flag);

    va_start(args, first_flag);
    while (1) {
        flag = va_arg(args, char*);
        if (flag == NULL) {break;}

        compiler_flags_add_flag(&flags, flag);
    }

    return flags;
}

bld_compiler_flags compiler_flags_copy(bld_compiler_flags* flags) {
    bld_compiler_flags cpy;
    bld_iter iter;
    bld_string* flag;

    cpy.flags = array_copy(&flags->flags);
    cpy.flag_hash = set_copy(&flags->flag_hash);
    cpy.removed = set_copy(&flags->removed);

    iter = iter_array(&cpy.flags);
    while (iter_next(&iter, (void**) &flag)) {
        bld_string temp = string_copy(flag);
        *flag = temp;
    }

    iter = iter_set(&cpy.removed);
    while (iter_next(&iter, (void**) &flag)) {
        bld_string temp = string_copy(flag);
        *flag = temp;
    }

    return cpy;
}

uintmax_t compiler_flags_hash(bld_compiler_flags* flags) {
    uintmax_t seed = 2346;
    bld_iter iter;
    bld_string* flag;

    iter = iter_array(&flags->flags);
    while (iter_next(&iter, (void**) &flag)) {
        seed = (seed << 5) + string_hash(string_unpack(flag));
    }

    iter = iter_set(&flags->removed);
    while (iter_next(&iter, (void**) &flag)) {
        seed = (seed << 5) + string_hash(string_unpack(flag));
    }

    return seed;
}

void compiler_flags_add_flag(bld_compiler_flags* flags, char* flag) {
    bld_string temp = string_pack(flag);
    uintmax_t hash = string_hash(flag);
    temp = string_copy(&temp);

    if (set_has(&flags->removed, hash)) {
        log_fatal("compiler_flags_add_flag: trying to add flag \"%s\" which has already been removed by this set of flags", flag);
    }

    array_push(&flags->flags, &temp);
    if (set_add(&flags->flag_hash, hash, NULL)) {
        log_fatal("compiler_flags_add_flag: tried to add flag \"%s\" twice", flag);
    }
}

void compiler_flags_remove_flag(bld_compiler_flags* flags, char* flag) {
    bld_string temp = string_pack(flag);
    uintmax_t hash = string_hash(flag);

    if (set_has(&flags->flag_hash, hash)) {
        log_fatal("compiler_flags_remove_flag: trying to remove flag \"%s\" which has already been added by this set of flags", flag);
    }

    temp = string_copy(&temp);
    if (set_add(&flags->removed, string_hash(flag), &temp)) {
        log_fatal("compiler_flags_remove_flag: trying to remove flag \"%s\" twice", flag);
    }
}

int parse_compiler(FILE* file, bld_compiler* compiler) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"executable", "flags"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_compiler_executable,
        (bld_parse_func) parse_compiler_flags,
    };

    compiler->flags = compiler_flags_new();
    amount_parsed = json_parse_map(file, compiler, size, parsed, (char**) keys, funcs);
    if (amount_parsed != size) {
        log_warn("parse_compiler: could not parse compiler");
        goto parse_failed;
    }

    return 0;
    parse_failed:
    if (parsed[0]) {
        string_free(&compiler->executable);
    }

    if (parsed[1]) {
        compiler_flags_free(&compiler->flags);
    }

    return -1;
}

int parse_compiler_executable(FILE* file, bld_compiler* compiler) {
    bld_string str;
    int result = string_parse(file, &str);
    if (result) {
        log_warn("Could not parse compiler executable");
        return -1;
    }

    compiler->executable = str;
    return result;
}

int parse_compiler_flags(FILE* file, bld_compiler* compiler) {
    int values;
    bld_compiler_flags flags;

    flags = compiler_flags_new();
    values = json_parse_array(file, &flags, (bld_parse_func) parse_compiler_flag);
    if (values < 0) {
        compiler_flags_free(&flags);
        log_warn("Could not parse compiler flags");
        return -1;
    }

    compiler->flags = flags;
    return 0;
}

int parse_compiler_flag(FILE* file, bld_compiler_flags* flags) {
    bld_string str;
    int result = string_parse(file, &str);
    if (result) {
        log_warn("Could not parse compiler flag");
        return -1;
    }

    array_push(&flags->flags, &str);

    return result;
}
