#include "../logging.h"
#include "../iter.h"
#include "../file.h"
#include "gcc.h"

bld_string bld_linker_string_gcc = STRING_COMPILE_TIME_PACK("gcc");

int linker_executable_make_gcc(bld_linker* linker, bld_path* root, bld_array* files, bld_array* flags, bld_path* name) {
    int error;
    bld_string cmd;
    bld_iter iter_files;
    bld_iter iter_flags;
    bld_file* file;
    bld_linker_flags* file_flags;

    cmd = string_copy(&linker->executable);

    if (files->size != flags->size) {
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

    string_append_string(&cmd, " -o ");
    string_append_string(&cmd, path_to_string(name));

    error = system(string_unpack(&cmd));

    string_free(&cmd);
    return error;
}
