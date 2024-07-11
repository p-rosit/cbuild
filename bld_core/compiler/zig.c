#include "../logging.h"
#include "../os.h"
#include "../iter.h"
#include "zig.h"

bld_string bld_compiler_string_zig = STRING_COMPILE_TIME_PACK("zig");

int compile_to_object_zig(bld_compiler* compiler, bld_path* file_path, bld_path* object_path) {
    int code;
    bld_string cmd;
    bld_path object_dir;
    bld_string name;

    cmd = string_copy(&compiler->executable);
    string_append_space(&cmd);
    string_append_string(&cmd, "build-obj ");

    string_append_string(&cmd, path_to_string(file_path));
    string_append_space(&cmd);

    {
        bld_iter iter;
        bld_string* flag;

        iter = iter_array(&compiler->flags.flags);
        while (iter_next(&iter, (void**) &flag)) {
            string_append_string(&cmd, string_unpack(flag));
            string_append_space(&cmd);
        }
    }

    {
        bld_path temp_path;

        object_dir = path_copy(object_path);
        temp_path = path_from_string(path_remove_last_string(&object_dir));
        path_remove_file_ending(&temp_path);
        name = temp_path.str;

    }

    string_append_string(&cmd, "--name ");
    string_append_string(&cmd, string_unpack(&name));

    {
        char cwd[FILENAME_MAX];
        if (!os_cwd(cwd, FILENAME_MAX)) {
            log_fatal(LOG_FATAL_PREFIX "could not get cwd");
        }

        os_set_cwd(path_to_string(&object_dir));
        code = system(string_unpack(&cmd));
        os_set_cwd(cwd);
    }

    {
        string_append_string(&name, ".o.o");
        path_append_string(&object_dir, string_unpack(&name));
        remove(path_to_string(&object_dir));
    }

    string_free(&cmd);
    string_free(&name);
    path_free(&object_dir);
    return code;
}

int compiler_file_is_implementation_zig(bld_string* name) {
    bld_string zig_extension;
    bld_string extension;

    zig_extension = string_pack("zig");
    extension = compiler_get_file_extension(name);

    return string_eq(&zig_extension, &extension);
}

int compiler_file_is_header_zig(bld_string* name) {
    (void)(name);
    return 0;
}

bld_language_type compiler_file_language_zig(bld_string* name) {
    bld_string zig_extension;
    bld_string extension;

    zig_extension = string_pack("zig");
    extension = compiler_get_file_extension(name);

    if (string_eq(&zig_extension, &extension)) {
        return BLD_LANGUAGE_ZIG;
    }

    log_fatal(LOG_FATAL_PREFIX "zig could not determine language of \"%s\"", string_unpack(name));
    return BLD_LANGUAGE_AMOUNT; /* unreachable */
}
