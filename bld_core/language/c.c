#include "../logging.h"
#include "../os.h"
#include "utils.h"
#include "c.h"

bld_string bld_language_string_c = STRING_COMPILE_TIME_PACK("c");

int generate_symbol_file(bld_project_base*, bld_file*, bld_path*);

int language_get_includes_c(bld_path* path, bld_file* file, bld_set* files) {
    size_t line_number;
    FILE* f;
    bld_path parent_path;
    bld_set* includes;

    if (file->type == BLD_FILE_DIRECTORY) {
        log_fatal(LOG_FATAL_PREFIX "cannot parse includes for directory \"%s\"", string_unpack(&file->name));
    }

    includes = file_includes_get(file);
    if (includes == NULL) {
        log_fatal(LOG_FATAL_PREFIX "attempting to parse includes of file which does not have includes: %d", file->type);
    }

    f = fopen(path_to_string(path), "r");
    if (f == NULL) {return -1;}

    parent_path = path_copy(path);
    path_remove_last_string(&parent_path);

    line_number = 0;
    while (1) {
        FILE* included_file;
        bld_string str;
        bld_path file_path;
        char c;

        if (!expect_char(f, '#')) {goto next_line;}
        if (!expect_string(f, "include")) {goto next_line;}
        if (!expect_char(f, '\"')) {goto next_line;}

        str = string_new();
        c = getc(f);
        while (c != EOF && c != '\"' && c != '\n') {
            string_append_char(&str, c);
            c = getc(f);
        }
        if (c == EOF) {string_free(&str); break;}
        if (c == '\n') {goto next_line;}

        file_path = path_copy(&parent_path);
        path_append_string(&file_path, string_unpack(&str));

        included_file = fopen(path_to_string(&file_path), "r");
        if (included_file == NULL) {
            log_warn("%s:%lu - Included file \"%s\" is not accessible, ignoring.", path_to_string(&file->path), line_number, string_unpack(&str));
            path_free(&file_path);
            string_free(&str);
            goto next_line;
        }
        fclose(included_file);

        {
            int exists;
            bld_file* included_file;
            bld_path path;

            included_file = set_get(files, file_get_id(&file_path));
            if (included_file == NULL) {
                log_fatal(LOG_FATAL_PREFIX "unreachable error");
            }
            path = path_from_string(path_to_string(&included_file->path));
            exists = set_add(includes, included_file->identifier.id, &path);
            if (exists) {
                log_warn("\"%s\" has duplicate import \"%s\"", path_to_string(&file->path), path_to_string(&included_file->path));
                path_free(&path);
            }
        }

        path_free(&file_path);
        string_free(&str);

        next_line:
        if (!skip_line(f)) {break;}
        line_number++;
    }

    path_free(&parent_path);
    fclose(f);

    return 0;
}

int language_get_symbols_c(bld_project_base* base, bld_path* path, bld_file* file) {
    FILE* f;
    int c, symbol_type;
    bld_string func;
    bld_path symbol_path;
    (void)(path);

    symbol_path = path_copy(&base->root);
    if (base->cache.loaded) {
        path_append_path(&symbol_path, &base->cache.root);
    }
    path_append_string(&symbol_path, "symbols.txt");

    {
        int error;

        error = generate_symbol_file(base, file, &symbol_path);
        if (error) {
            return -1;
        }
    }

    f = fopen(path_to_string(&symbol_path), "r");
    if (f == NULL) {log_fatal(LOG_FATAL_PREFIX "symbol file could not be opened");}

    while (1) {
        c = fgetc(f);
        if (c == EOF) {
            log_warn("Unexpected EOF when parsing symbols of \"%s\"", path_to_string(&file->path));
            break;
        }

        while (c != EOF && c != ' ') {c = fgetc(f);}
        if (c == EOF) {
            log_warn("Unexpected EOF when parsing symbols of \"%s\"", path_to_string(&file->path));
            break;
        }
        while (c != EOF && c == ' ') {c = fgetc(f);}
        if (c == EOF) {
            log_warn("Unexpected EOF when parsing symbols of \"%s\"", path_to_string(&file->path));
            break;
        }

        symbol_type = c;
        if (symbol_type != 'T' && symbol_type != 'B' && symbol_type != 'R' && symbol_type != 'D' && symbol_type != 'S' && symbol_type != 'U') {goto next_line;}

        c = fgetc(f);
        if (c != ' ') {
            log_warn("Unexpected character \'%c\' when parsing symbols, expected \' \'", c);
            goto next_line;
        }

        func = string_new();
        c = fgetc(f);
        while (c != '\n' && c != ' ' && c != EOF) {
            string_append_char(&func, c);
            c = fgetc(f);
        }

        if (symbol_type == 'T' || symbol_type == 'B' || symbol_type == 'R' || symbol_type == 'D' || symbol_type == 'S') {
            bld_set* defined;
            defined = file_defined_get(file);
            if (defined != NULL) {
                set_add(defined, string_hash(string_unpack(&func)), &func);
            } else {
                string_free(&func);
            }
        } else if (symbol_type == 'U') {
            bld_set* undefined;
            undefined = file_undefined_get(file);
            if (undefined == NULL) {
                log_fatal(LOG_FATAL_PREFIX "parsing symbols for file type %d which has no undefined symbols", file->type);
            }

            set_add(undefined, string_hash(string_unpack(&func)), &func);
        } else {
            log_fatal(LOG_FATAL_PREFIX "unreachable error");
        }

        next_line:
        while (c != '\n' && c != EOF) {c = fgetc(f);}
        c = fgetc(f);
        if (c == EOF) {break;}
    }

    fclose(f);

    remove(path_to_string(&symbol_path));
    path_free(&symbol_path);
    return 0;
}

int generate_symbol_file(bld_project_base* base, bld_file* file, bld_path* symbol_path) {
    FILE* sym;
    int error;
    bld_path object_path;
    bld_string object_name;
    bld_string cmd;

    object_path = path_copy(&base->root);
    if (base->cache.loaded) {
        path_append_path(&object_path, &base->cache.root);
    }

    object_name = file_object_name(file);
    string_append_string(&object_name, ".o");
    path_append_string(&object_path, string_unpack(&object_name));

    if (!os_file_exists(path_to_string(&object_path))) {
        return -1;
    }

    sym = fopen(path_to_string(symbol_path), "w");
    if (sym == NULL) {
        log_fatal(LOG_FATAL_PREFIX "could not create symbol file");
    }
    fclose(sym);

    cmd = string_new();
    string_append_string(&cmd, "nm ");
    string_append_string(&cmd, path_to_string(&object_path));
    string_append_string(&cmd, " >> ");
    string_append_string(&cmd, path_to_string(symbol_path));

    error = system(string_unpack(&cmd));

    string_free(&object_name);
    path_free(&object_path);
    string_free(&cmd);
    return error;
}
