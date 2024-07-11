#include <string.h>
#include <stdarg.h>
#include "logging.h"
#include "json.h"
#include "compiler.h"

bld_compiler compiler_new(bld_compiler_type type, char* executable) {
    bld_compiler compiler;
    bld_string str;

    str = string_pack(executable);
    compiler.type = type;
    compiler.executable = string_copy(&str);
    compiler.flags = compiler_flags_new();

    return compiler;
}

void compiler_free(bld_compiler* compiler) {
    if (compiler == NULL) {return;}
    
    string_free(&compiler->executable);
    compiler_flags_free(&compiler->flags);
}

void compiler_assembled_free(bld_compiler* compiler) {
    if (compiler == NULL) {return;}

    array_free(&compiler->flags.flags);
    set_free(&compiler->flags.flag_hash);
    set_free(&compiler->flags.removed);
}

bld_compiler compiler_copy(bld_compiler* compiler) {
    bld_compiler cpy;

    cpy.type = compiler->type;
    cpy.executable = string_copy(&compiler->executable);
    cpy.flags = compiler_flags_copy(&compiler->flags);

    return cpy;
}

uintmax_t compiler_hash(bld_compiler* compiler) {
    uintmax_t seed;

    seed = 2349;
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
    uintmax_t seed;
    bld_iter iter;
    bld_string* flag;

    seed = 2346;
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
    bld_string temp;
    uintmax_t hash;

    hash = string_hash(flag);
    temp = string_pack(flag);
    temp = string_copy(&temp);

    if (set_has(&flags->removed, hash)) {
        log_fatal(LOG_FATAL_PREFIX "trying to add flag \"%s\" which has already been removed by this set of flags", flag);
    }

    array_push(&flags->flags, &temp);
    if (set_add(&flags->flag_hash, hash, NULL)) {
        log_fatal(LOG_FATAL_PREFIX "tried to add flag \"%s\" twice", flag);
    }
}

void compiler_flags_remove_flag(bld_compiler_flags* flags, char* flag) {
    bld_string temp;
    uintmax_t hash;

    temp = string_pack(flag);
    hash = string_hash(flag);

    if (set_has(&flags->flag_hash, hash)) {
        log_fatal(LOG_FATAL_PREFIX "trying to remove flag \"%s\" which has already been added by this set of flags", flag);
    }

    temp = string_copy(&temp);
    if (set_add(&flags->removed, string_hash(flag), &temp)) {
        log_fatal(LOG_FATAL_PREFIX "trying to remove flag \"%s\" twice", flag);
    }
}

void compiler_flags_expand(bld_string* cmd, bld_array* flags) {
    bld_array flags_added;
    bld_set flags_removed;
    bld_iter iter;
    bld_compiler_flags* f;
    bld_string* str;

    flags_added = array_new(sizeof(bld_string));
    flags_removed = set_new(sizeof(int));

    array_reverse(flags);
    iter = iter_array(flags);
    while (iter_next(&iter, (void**) &f)) {
        bld_iter iter;
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
            if (amount == NULL) {
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

void serialize_compiler(FILE* cache, bld_compiler* compiler, int depth) {
    fprintf(cache, "{\n");

    json_serialize_key(cache, "type", depth);
    fprintf(cache, "\"%s\"", string_unpack(compiler_get_string(compiler->type)));
    fprintf(cache, ",\n");

    json_serialize_key(cache, "executable", depth);
    fprintf(cache, "\"%s\"", string_unpack(&compiler->executable));
    fprintf(cache, ",\n");

    json_serialize_key(cache, "flags", depth);
    serialize_compiler_flags(cache, &compiler->flags, depth + 1);

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_compiler_flags(FILE* cache, bld_compiler_flags* flags, int depth) {
    fprintf(cache, "{\n");

    json_serialize_key(cache, "added", depth);
    serialize_compiler_flags_added_flags(cache, flags, depth + 1);
    fprintf(cache, ",\n");

    json_serialize_key(cache, "removed", depth);
    serialize_compiler_flags_removed_flags(cache, flags, depth + 1);

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_compiler_flags_added_flags(FILE* cache, bld_compiler_flags* flags, int depth) {
    int first = 1;
    bld_iter iter;
    bld_string* flag;

    fprintf(cache, "[");
    if (flags->flags.size > 1) {
        fprintf(cache, "\n");
    }

    iter = iter_array(&flags->flags);
    while (iter_next(&iter, (void**) &flag)) {
        if (!first) {fprintf(cache, ",\n");}
        else {first = 0;}
        if (flags->flags.size > 1) {
            fprintf(cache, "%*c", 2 * depth, ' ');
        }
        fprintf(cache, "\"%s\"", string_unpack(flag));
    }

    if (flags->flags.size > 1) {
        fprintf(cache, "\n%*c", 2 * (depth - 1), ' ');
    }
    fprintf(cache, "]");
}

void serialize_compiler_flags_removed_flags(FILE* cache, bld_compiler_flags* flags, int depth) {
    int first = 1;
    bld_iter iter;
    bld_string* flag;

    fprintf(cache, "[");
    if (flags->removed.size > 1) {
        fprintf(cache, "\n");
    }

    iter = iter_set(&flags->removed);
    while (iter_next(&iter, (void**) &flag)) {
        if (!first) {fprintf(cache, ",\n");}
        else {first = 0;}
        if (flags->removed.size > 1) {
            fprintf(cache, "%*c", 2 * (depth + 1), ' ');
        }
        fprintf(cache, "\"%s\"", string_unpack(flag));
    }

    if (flags->removed.size > 1) {
        fprintf(cache, "\n%*c", 2 * depth, ' ');
    }
    fprintf(cache, "]");
}

int parse_compiler(FILE* file, bld_compiler* compiler) {
    int amount_parsed;
    int size = 3;
    int parsed[3];
    char *keys[3] = {"type", "executable", "flags"};
    bld_parse_func funcs[3] = {
        (bld_parse_func) parse_compiler_type,
        (bld_parse_func) parse_compiler_executable,
        (bld_parse_func) parse_compiler_compiler_flags,
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

int parse_compiler_type(FILE* file, bld_compiler* compiler) {
    bld_string str;
    int error;

    error = string_parse(file, &str);
    if (error) {
        log_warn("Could not parse compiler executable");
        return -1;
    }

    compiler->type = compiler_get_mapping(&str);
    string_free(&str);
    return compiler->type == BLD_COMPILER_AMOUNT;
}

int parse_compiler_executable(FILE* file, bld_compiler* compiler) {
    bld_string str;
    int error;

    error = string_parse(file, &str);
    if (error) {
        log_warn("Could not parse compiler executable");
        return -1;
    }

    compiler->executable = str;
    return error;
}

int parse_compiler_compiler_flags(FILE* file, bld_compiler* compiler) {
    int error;

    error = parse_compiler_flags(file, &compiler->flags);
    if (error) {
        log_warn("parse_compiler_compiler_flags: could not parse flags");
        return error;
    }

    return error;
}

int parse_compiler_flags(FILE* file, bld_compiler_flags* flags) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"added", "removed"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_compiler_flags_added_flags,
        (bld_parse_func) parse_compiler_flags_removed_flags,
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
        bld_iter iter;
        bld_string* flag;

        iter = iter_array(&flags->flags);
        while (iter_next(&iter, (void**) &flag)) {
            string_free(flag);
        }
        array_free(&flags->flags);
        set_free(&flags->flag_hash);
    }

    if (parsed[1]) {
        bld_iter iter;
        bld_string* flag;

        iter = iter_set(&flags->removed);
        while (iter_next(&iter, (void**) &flag)) {
            string_free(flag);
        }
        set_free(&flags->removed);
    }

    return -1;
}

int parse_compiler_flags_added_flags(FILE* file, bld_compiler_flags* flags) {
    int values;

    values = json_parse_array(file, flags, (bld_parse_func) parse_compiler_flags_added_flag);
    if (values < 0) {
        log_warn("Could not parse compiler flags");
        return -1;
    }

    return 0;
}

int parse_compiler_flags_added_flag(FILE* file, bld_compiler_flags* flags) {
    bld_string flag;
    uintmax_t hash;
    int error;

    error = string_parse(file, &flag);
    if (error) {
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

int parse_compiler_flags_removed_flags(FILE* file, bld_compiler_flags* flags) {
    int values;

    values = json_parse_array(file, flags, (bld_parse_func) parse_compiler_flags_removed_flag);
    if (values < 0) {
        log_warn("Could not parse compiler flags");
        return -1;
    }

    return 0;
}

int parse_compiler_flags_removed_flag(FILE* file, bld_compiler_flags* flags) {
    bld_string flag;
    uintmax_t hash;
    int error;

    error = string_parse(file, &flag);
    if (error) {
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

    set_add(&flags->removed, hash, &flag);

    return 0;
    parse_failed:
    string_free(&flag);
    return -1;
}
