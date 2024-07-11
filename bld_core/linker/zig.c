#include "../logging.h"
#include "../iter.h"
#include "../file.h"
#include "zig.h"

bld_string bld_linker_string_zig = STRING_COMPILE_TIME_PACK("zig");

int linker_executable_make_zig(bld_linker* linker, bld_path* root, bld_array* files, bld_array* flags, bld_path* name) {
    int error;
    bld_string cmd;
    bld_iter iter_files;
    bld_iter iter_flags;
    bld_file* file;
    bld_linker_flags* file_flags;
    bld_path executable_dir;
    bld_string executable_name;

    cmd = string_copy(&linker->executable);
    string_append_string(&cmd, " build-exe");

    if (files->size != flags->size + 1) {
        log_fatal(LOG_FATAL_PREFIX "equal amounts of file and flag entires required");
    }

    iter_files = iter_array(files);
    iter_flags = iter_array(flags);
    while (iter_next(&iter_files, (void**) &file) && iter_next(&iter_flags, (void**) &file_flags)) {
        bld_path object_path;
        bld_string object_name;

        object_path = path_copy(root);
        object_name = file_object_name(file);
        string_append_string(&object_name, ".o");

        path_append_string(&object_path, string_unpack(&object_name));
        string_free(&object_name);

        string_append_space(&cmd);
        string_append_string(&cmd, path_to_string(&object_path));
        path_free(&object_path);

        string_append_space(&cmd);
        // TODO: append flags
        string_append_string(&cmd, string_unpack(file_flags));
    }

    {
        int exists;

        exists = iter_next(&iter_flags, (void**) &file_flags);
        if (!exists) {
            log_fatal(LOG_FATAL_PREFIX "missing final entry");
        }

        // TODO: append flags
    }

    {
        bld_path temp_path;

        executable_dir = path_copy(name);
        temp_path = path_from_string(path_remove_last_string(&executable_dir));
        path_remove_file_ending(&temp_path);
        executable_name = temp_path.str;
    }

    string_append_string(&cmd, " --name ");
    string_append_string(&cmd, string_unpack(&executable_name));

    {
        char cwd[FILENAME_MAX];
        if (!os_cwd(cwd, FILENAME_MAX)) {
            log_fatal(LOG_FATAL_PREFIX "could not get cwd");
        }

        os_set_cwd(path_to_string(&executable_dir));
        error = system(string_unpack(&cmd));
        os_set_cwd(cwd);
    }

    string_free(&cmd);
    string_free(&executable_name);
    path_free(&executable_dir);
    return error;
}
