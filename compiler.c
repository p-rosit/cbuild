#include <string.h>
#include <stdarg.h>
#include "logging.h"
#include "json.h"
#include "compiler.h"

bit_compiler compiler_new(char* executable) {
    bit_compiler compiler;
    bit_string str;

    str = string_pack(executable);
    compiler.executable = string_copy(&str);
    compiler.flags = compiler_flags_new();

    return compiler;
}

bit_compiler compiler_with_flags(char* executable, ...) {
    bit_compiler compiler;
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

void compiler_free(bit_compiler* compiler) {
    if (compiler == NULL) {return;}
    
    string_free(&compiler->executable);
    compiler_flags_free(&compiler->flags);
}

bit_compiler compiler_copy(bit_compiler* compiler) {
    bit_compiler cpy;

    cpy.executable = string_copy(&compiler->executable);
    cpy.flags = compiler_flags_copy(&compiler->flags);

    return cpy;
}

uintmax_t compiler_hash(bit_compiler* compiler) {
    uintmax_t seed = 2349;

    seed = (seed << 5) + string_hash(string_unpack(&compiler->executable));
    seed = (seed << 5) + compiler_flags_hash(&compiler->flags);

    return seed;
}

void compiler_add_flag(bit_compiler* compiler, char* flag) {
    compiler_flags_add_flag(&compiler->flags, flag);
}

void compiler_remove_flag(bit_compiler* compiler, char* flag) {
    compiler_flags_remove_flag(&compiler->flags, flag);
}

bit_compiler_flags compiler_flags_new(void) {
    bit_compiler_flags flags;
    flags.flags = array_new(sizeof(bit_string));
    flags.flag_hash = set_new(0);
    flags.removed = set_new(sizeof(bit_string));
    return flags;
}

void compiler_flags_free(bit_compiler_flags* flags) {
    bit_iter iter;
    bit_string* flag;

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

bit_compiler_flags compiler_flags_with_flags(char* first_flag, ...) {
    bit_compiler_flags flags;
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

bit_compiler_flags compiler_flags_copy(bit_compiler_flags* flags) {
    bit_compiler_flags cpy;
    bit_iter iter;
    bit_string* flag;

    cpy.flags = array_copy(&flags->flags);
    cpy.flag_hash = set_copy(&flags->flag_hash);
    cpy.removed = set_copy(&flags->removed);

    iter = iter_array(&cpy.flags);
    while (iter_next(&iter, (void**) &flag)) {
        bit_string temp = string_copy(flag);
        *flag = temp;
    }

    iter = iter_set(&cpy.removed);
    while (iter_next(&iter, (void**) &flag)) {
        bit_string temp = string_copy(flag);
        *flag = temp;
    }

    return cpy;
}

uintmax_t compiler_flags_hash(bit_compiler_flags* flags) {
    uintmax_t seed = 2346;
    bit_iter iter;
    bit_string* flag;

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

void compiler_flags_add_flag(bit_compiler_flags* flags, char* flag) {
    bit_string temp = string_pack(flag);
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

void compiler_flags_remove_flag(bit_compiler_flags* flags, char* flag) {
    bit_string temp = string_pack(flag);
    uintmax_t hash = string_hash(flag);

    if (set_has(&flags->flag_hash, hash)) {
        log_fatal("compiler_flags_remove_flag: trying to remove flag \"%s\" which has already been added by this set of flags", flag);
    }

    temp = string_copy(&temp);
    if (set_add(&flags->removed, string_hash(flag), &temp)) {
        log_fatal("compiler_flags_remove_flag: trying to remove flag \"%s\" twice", flag);
    }
}

void compiler_flags_expand(bit_string* cmd, bit_array* flags) {
    bit_array flags_added = array_new(sizeof(bit_string));
    bit_set flags_removed = set_new(sizeof(int));
    bit_iter iter = iter_array(flags);
    bit_compiler_flags* f;
    bit_string* str;

    array_reverse(flags);
    while (iter_next(&iter, (void**) &f)) {
        bit_iter iter;
        uintmax_t hash;
        int* amount;
        array_reverse(&f->flags);

        iter = iter_array(&f->flags);
        while (iter_next(&iter, (void**) &str)) {
            hash = string_hash(string_unpack(str));
            amount = set_get(&flags_removed, hash);
            if (amount == NULL) {
                array_push(&flags_added, str);
            } else {
                *amount -= 1;
                if (*amount == 0) {
                    set_remove(&flags_removed, hash);
                }
            }
        }

        iter = iter_set(&f->removed);
        while (iter_next(&iter, (void**) &str)) {
            hash = string_hash(string_unpack(str));

            amount = set_get(&flags_removed, hash);
            if (amount != NULL) {
                int new = 1;
                set_add(&flags_removed, hash, &new);
            } else {
                *amount += 1;
            }
        }

        array_reverse(&f->flags);
    }
    array_reverse(flags);

    array_reverse(&flags_added);
    iter = iter_array(&flags_added);
    while (iter_next(&iter, (void**) &str)) {
        string_append_space(cmd);
        string_append_string(cmd, string_unpack(str));
    }

    array_free(&flags_added);
    set_free(&flags_removed);
}

int parse_compiler(FILE* file, bit_compiler* compiler) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"executable", "flags"};
    bit_parse_func funcs[2] = {
        (bit_parse_func) parse_compiler_executable,
        (bit_parse_func) parse_compiler_compiler_flags,
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

int parse_compiler_executable(FILE* file, bit_compiler* compiler) {
    bit_string str;
    int result = string_parse(file, &str);
    if (result) {
        log_warn("Could not parse compiler executable");
        return -1;
    }

    compiler->executable = str;
    return result;
}

int parse_compiler_compiler_flags(FILE* file, bit_compiler* compiler) {
    int result;
    result = parse_compiler_flags(file, &compiler->flags);
    if (result) {
        log_warn("parse_compiler_compiler_flags: could not parse flags");
        return result;
    }

    return result;
}

int parse_compiler_flags(FILE* file, bit_compiler_flags* flags) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"added", "removed"};
    bit_parse_func funcs[2] = {
        (bit_parse_func) parse_compiler_flags_added_flags,
        (bit_parse_func) parse_compiler_flags_removed_flags,
    };

    *flags = compiler_flags_new();
    amount_parsed = json_parse_map(file, flags, size, parsed, (char**) keys, funcs);
    if (amount_parsed != size) {
        log_warn("parse_compiler_flags: could not parse compiler flags");
        goto parse_failed;
    }

    return 0;
    parse_failed:
    if (parsed[0]) {
        bit_iter iter;
        bit_string* flag;

        iter = iter_array(&flags->flags);
        while (iter_next(&iter, (void**) &flag)) {
            string_free(flag);
        }
        array_free(&flags->flags);
        set_free(&flags->flag_hash);
    }

    if (parsed[1]) {
        bit_iter iter;
        bit_string* flag;

        iter = iter_set(&flags->removed);
        while (iter_next(&iter, (void**) &flag)) {
            string_free(flag);
        }
        set_free(&flags->removed);
    }

    return -1;
}

int parse_compiler_flags_added_flags(FILE* file, bit_compiler_flags* flags) {
    int values;

    values = json_parse_array(file, flags, (bit_parse_func) parse_compiler_flags_added_flag);
    if (values < 0) {
        log_warn("Could not parse compiler flags");
        return -1;
    }

    return 0;
}

int parse_compiler_flags_added_flag(FILE* file, bit_compiler_flags* flags) {
    bit_string flag;
    uintmax_t hash;
    int result = string_parse(file, &flag);
    if (result) {
        log_warn("Could not parse added flag");
        return -1;
    }

    hash = string_hash(string_unpack(&flag));
    if (set_has(&flags->flag_hash, hash)) {
        log_warn("parse_compiler_flags_added_flag: duplicate flag, \"%s\"", string_unpack(&flag));
        goto parse_failed;
    }

    if (set_has(&flags->removed, hash)) {
        log_warn("parse_compiler_flags_added_flag: flag exists in both lists \"%s\"", string_unpack(&flag));
        goto parse_failed;
    }

    array_push(&flags->flags, &flag);
    set_add(&flags->flag_hash, hash, NULL);

    return 0;
    parse_failed:
    string_free(&flag);
    return -1;
}

int parse_compiler_flags_removed_flags(FILE* file, bit_compiler_flags* flags) {
    int values;

    values = json_parse_array(file, flags, (bit_parse_func) parse_compiler_flags_removed_flags);
    if (values < 0) {
        log_warn("Could not parse compiler flags");
        return -1;
    }

    return 0;
}

int parse_compiler_flags_removed_flag(FILE* file, bit_compiler_flags* flags) {
    bit_string flag;
    uintmax_t hash;
    int result = string_parse(file, &flag);
    if (result) {
        log_warn("Could not parse removed flag");
        return -1;
    }

    hash = string_hash(string_unpack(&flag));
    if (set_has(&flags->flag_hash, hash)) {
        log_warn("parse_compiler_flags_removed_flag: flag exists in both lists, \"%s\"", string_unpack(&flag));
        goto parse_failed;
    }

    if (set_has(&flags->removed, hash)) {
        log_warn("parse_compiler_flags_removed_flag: duplicate flag \"%s\"", string_unpack(&flag));
        goto parse_failed;
    }

    set_add(&flags->flag_hash, hash, &flag);

    return 0;
    parse_failed:
    string_free(&flag);
    return -1;
}
