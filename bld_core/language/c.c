#include "../logging.h"
#include "utils.h"
#include "c.h"

bld_string bld_language_string_c = STRING_COMPILE_TIME_PACK("c");

int language_get_includes_c(bld_project_base* base, bld_path* path, bld_file* file, bld_set* files) {
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
