#include <stdarg.h>
#include "dstr.h"
#include "linker.h"
#include "linker/linker.h"
#include "logging.h"
#include "json.h"

bld_linker linker_new(bld_linker_type type, char* executable) {
    bld_linker linker;
    bld_string str;

    str = string_pack(executable);
    linker.type = type;
    linker.executable = string_copy(&str);
    linker.flags = linker_flags_new();
    
    return linker;
}

void linker_free(bld_linker* linker) {
    string_free(&linker->executable);
    linker_flags_free(&linker->flags);
}

bld_linker linker_copy(bld_linker* linker) {
    bld_linker cpy;

    cpy.type = linker->type;
    cpy.executable = string_copy(&linker->executable);
    cpy.flags = linker_flags_copy(&linker->flags);

    return cpy;
}

void linker_add_flag(bld_linker* linker, char* flag) {
    linker_flags_add_flag(&linker->flags, flag);
}

bld_linker_flags linker_flags_new(void) {
    bld_linker_flags flags;
    flags.flags = array_new(sizeof(bld_string));
    return flags;
}

void linker_flags_free(bld_linker_flags* flags) {
    bld_iter iter;
    bld_string* flag;

    iter = iter_array(&flags->flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(flag);
    }
    array_free(&flags->flags);
}

void linker_flags_assembled_free(bld_linker_flags* flags) {
    array_free(&flags->flags);
}

bld_linker_flags linker_flags_copy(bld_linker_flags* flags) {
    bld_string str;
    bld_string *flag;
    bld_iter iter;
    bld_linker_flags cpy;

    cpy.flags = array_copy(&flags->flags);

    iter = iter_array(&cpy.flags);
    while (iter_next(&iter, (void**) &flag)) {
        str = string_copy(flag);
        *flag = str;
    }

    return cpy;
}

void linker_flags_add_flag(bld_linker_flags* linker, char* flag) {
    bld_string str;

    str = string_pack(flag);
    str = string_copy(&str);
    array_push(&linker->flags, &str);
}

uintmax_t linker_flags_hash(bld_linker_flags* linker_flags) {
    uintmax_t seed;
    bld_iter iter;
    bld_string* flag;

    seed = 335545;
    iter = iter_array(&linker_flags->flags);
    while (iter_next(&iter, (void**) &flag)) {
        seed = (seed << 7) + string_hash(string_unpack(flag));
    }

    return seed;
}

void linker_flags_expand(bld_string* str, bld_array* linker_flags) {
    bld_iter iter;
    bld_linker_flags* flags;
    array_reverse(linker_flags);

    iter = iter_array(linker_flags);
    while (iter_next(&iter, (void**) &flags)) {
        linker_flags_append(str, flags);
    }

    array_reverse(linker_flags);
}

void linker_flags_append(bld_string* str, bld_linker_flags* flags) {
    bld_iter iter;
    bld_string* f;
    array_reverse(&flags->flags);

    iter = iter_array(&flags->flags);
    while (iter_next(&iter, (void**) &f)) {
        string_append_space(str);
        string_append_string(str, string_unpack(f));
    }

    array_reverse(&flags->flags);
}

void serialize_linker(FILE* cache, bld_linker* linker, int depth) {
    fprintf(cache, "{\n");

    json_serialize_key(cache, "type", depth);
    fprintf(cache, "\"%s\"", string_unpack(linker_get_string(linker->type)));

    fprintf(cache, ",\n");
    json_serialize_key(cache, "executable", depth);
    fprintf(cache, "\"%s\"", string_unpack(&linker->executable));

    if (linker->flags.flags.size > 0) {
        fprintf(cache, ",\n");
        json_serialize_key(cache, "flags", depth);
        serialize_linker_flags(cache, &linker->flags, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_linker_flags(FILE* cache, bld_linker_flags* flags, int depth) {
    bld_string* flag;
    int first;
    bld_iter iter;

    fprintf(cache, "[");
    if (flags->flags.size > 1) {
        fprintf(cache, "\n");
    }

    first = 1;
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

int parse_linker(FILE* file, bld_linker* linker) {
    int amount_parsed;
    int size = 3;
    int parsed[3];
    char *keys[3] = {"type", "executable", "flags"};
    bld_parse_func funcs[3] = {
        (bld_parse_func) parse_linker_type,
        (bld_parse_func) parse_linker_executable,
        (bld_parse_func) parse_linker_linker_flags,
    };

    linker->flags = linker_flags_new();
    amount_parsed = json_parse_map(file, linker, size, parsed, (char**) keys, funcs);

    if (amount_parsed < size && !(parsed[0] && parsed[1])) {
        log_fatal(LOG_FATAL_PREFIX "free correctly");
        return -1;
    }
    return 0;
}

int parse_linker_type(FILE* file, bld_linker* linker) {
    bld_string str;
    int error;

    error = string_parse(file, &str);
    if (error) {
        log_warn("Could not parse linker executable");
        return -1;
    }

    linker->type = linker_get_mapping(&str);
    string_free(&str);
    return linker->type == BLD_LINKER_AMOUNT;
}

int parse_linker_executable(FILE* file, bld_linker* linker) {
    bld_string str;
    int error;

    error = string_parse(file, &str);
    if (error) {
        log_warn("Could not parse linker executable");
        return -1;
    }

    linker->executable = str;
    return error;
}

int parse_linker_linker_flags(FILE* file, bld_linker* linker) {
    bld_linker_flags flags;
    int error;

    error = parse_linker_flags(file, &flags);
    if (error) {
        linker_flags_free(&flags);
        log_warn("Could not parse linker flags");
    }

    linker->flags = flags;
    return error;
}

int parse_linker_flags(FILE* file, bld_linker_flags* flags) {
    int values;

    *flags = linker_flags_new();
    values = json_parse_array(file, flags, (bld_parse_func) parse_linker_flag);
    if (values < 0) {
        log_warn("Could not parse linker flags");
        log_fatal("Free correctly");
        return -1;
    }

    return 0;
}

int parse_linker_flag(FILE* file, bld_linker_flags* flags) {
    bld_string str;
    int error;

    error = string_parse(file, &str);
    if (error) {
        log_warn("Could not parse linker flag");
        return -1;
    }

    array_push(&flags->flags, &str);

    return error;
}
