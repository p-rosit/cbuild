#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "init.h"
#include "compiler.h"

const bld_string bld_command_string_compiler = STRING_COMPILE_TIME_PACK("compiler");

bld_target_build_information command_compiler_initial_setup(bld_command_compiler*, bld_data*);

int command_compiler(bld_command_compiler* cmd, bld_data* data) {
    bld_target_build_information flags;
    bld_target_build_information* file;

    flags = command_compiler_initial_setup(cmd, data);
    utils_apply_build_information(data, &flags);

    file = utils_get_build_info_for(data, &cmd->path);
    if (file == NULL) {
        log_fatal("Requested file '%s' does not exist in the project", path_to_string(&cmd->path));
    }

    if (cmd->type == BLD_COMMAND_COMPILER_PRINT) {
        printf("Target:   %s\n", string_unpack(&cmd->target));
        printf("File:     %s\n", path_to_string(&cmd->path));

        if (file->info.compiler_set) {
            bld_compiler_flags* flags;
            bld_iter iter;
            bld_string* flag;

            if (file->info.compiler.type == BLD_COMPILER) {
                flags = &file->info.compiler.as.compiler.flags;

                printf("Compiler: %s\n", string_unpack(&file->info.compiler.as.compiler.executable));
            } else {
                flags = &file->info.compiler.as.flags;
            }

            if (flags->flags.size > 0 || flags->removed.size > 0) {
                printf("Flags:\n");
            }

            if (flags->flags.size > 0) {
                int first = 1;
                printf("    Added:   [");

                iter = iter_array(&flags->flags);
                while (iter_next(&iter, (void**) &flag)) {
                    if (!first) {
                        printf(", ");
                    }
                    first = 0;
                    printf("%s", string_unpack(flag));
                }
                printf("]\n");
            }

            if (flags->removed.size > 0) {
                int first = 1;
                printf("    Removed: [");

                iter = iter_set(&flags->removed);
                while (iter_next(&iter, (void**) &flag)) {
                    if (!first) {
                        printf(", ");
                    }
                    first = 0;
                    printf("%s", string_unpack(flag));
                }
                printf("]\n");
            }

            if (file->info.compiler.type != BLD_COMPILER && flags->flags.size == 0 && flags->removed.size == 0) {
                printf("No compiler info set\n");
            }
        } else {
            printf("No compiler info set\n");
        }
    } else if (cmd->type == BLD_COMMAND_COMPILER_CLEAR) {
        if (file->info.compiler_set) {
            bld_iter iter;
            bld_string* flag;
            bld_compiler_flags* flags;

            if (file->info.compiler.type == BLD_COMPILER) {
                string_free(&file->info.compiler.as.compiler.executable);
                flags = &file->info.compiler.as.compiler.flags;
            } else {
                flags = &file->info.compiler.as.flags;
            }

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

        file->info.compiler_set = 0;
    } else if (cmd->type == BLD_COMMAND_COMPILER_SET_COMPILER) {
        if (file->info.compiler_set) {
            bld_iter iter;
            bld_string* flag;
            bld_compiler_flags* flags;

            if (file->info.compiler.type == BLD_COMPILER) {
                string_free(&file->info.compiler.as.compiler.executable);
                flags = &file->info.compiler.as.compiler.flags;
            } else {
                flags = &file->info.compiler.as.flags;
            }

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

        file->info.compiler_set = 1;
        file->info.compiler.type = BLD_COMPILER;
        file->info.compiler.as.compiler = compiler_new(compiler_get_mapping(&cmd->compiler), string_unpack(&cmd->compiler));

        if (file->info.compiler.as.compiler.type == BLD_COMPILER_AMOUNT) {
            log_fatal("command_compiler: allow custom compiler");
        }

        {
            bld_iter iter;
            bld_command_flag* flag;
            bld_compiler_flags* flags;

            flags = &file->info.compiler.as.compiler.flags;

            iter = iter_array(&cmd->flags);
            while (iter_next(&iter, (void**) &flag)) {
                bld_string temp;

                temp = string_new();
                if (flag->is_switch) {
                    string_append_string(&temp, "-");
                } else {
                    string_append_string(&temp, "--");
                }
                string_append_string(&temp, string_unpack(&flag->flag));

                compiler_flags_add_flag(flags, string_unpack(&temp));
                string_free(&temp);
            }
        }
    } else if (cmd->type == BLD_COMMAND_COMPILER_ADD_FLAGS) {
        bld_iter iter;
        bld_compiler_flags* flags;
        bld_command_flag* flag;

        if (file->info.compiler_set) {
            bld_iter iter;
            bld_string* flag;

            if (file->info.compiler.type == BLD_COMPILER) {
                flags = &file->info.compiler.as.compiler.flags;
            } else {
                flags = &file->info.compiler.as.flags;
            }

            iter = iter_array(&flags->flags);
            while (iter_next(&iter, (void**) &flag)) {
                string_free(flag);
            }
            array_free(&flags->flags);
            set_free(&flags->flag_hash);
        } else {
            file->info.compiler_set = 1;
            file->info.compiler.type = BLD_COMPILER_FLAGS;
            file->info.compiler.as.flags.removed = set_new(sizeof(bld_string));

            flags = &file->info.compiler.as.flags;
        }

        flags->flags = array_new(sizeof(bld_string));
        flags->flag_hash = set_new(0);
        iter = iter_array(&cmd->flags);
        while (iter_next(&iter, (void**) &flag)) {
            bld_string temp;

            temp = string_new();
            if (flag->is_switch) {
                string_append_string(&temp, "-");
            } else {
                string_append_string(&temp, "--");
            }
            string_append_string(&temp, string_unpack(&flag->flag));

            compiler_flags_add_flag(flags, string_unpack(&temp));
            string_free(&temp);
        }
    } else if (cmd->type == BLD_COMMAND_COMPILER_REMOVE_FLAGS) {
        bld_iter iter;
        bld_compiler_flags* flags;
        bld_command_flag* flag;

        if (file->info.compiler_set) {
            bld_iter iter;
            bld_string* flag;

            if (file->info.compiler.type == BLD_COMPILER) {
                flags = &file->info.compiler.as.compiler.flags;
            } else {
                flags = &file->info.compiler.as.flags;
            }

            iter = iter_set(&flags->removed);
            while (iter_next(&iter, (void**) &flag)) {
                string_free(flag);
            }
            set_free(&flags->removed);
        } else {
            file->info.compiler_set = 1;
            file->info.compiler.type = BLD_COMPILER_FLAGS;
            file->info.compiler.as.flags.flags = array_new(sizeof(bld_string));
            file->info.compiler.as.flags.flag_hash = set_new(0);

            flags = &file->info.compiler.as.flags;
        }

        flags->removed = set_new(sizeof(bld_string));
        iter = iter_array(&cmd->flags);
        while (iter_next(&iter, (void**) &flag)) {
            bld_string temp;

            temp = string_new();
            if (flag->is_switch) {
                string_append_string(&temp, "-");
            } else {
                string_append_string(&temp, "--");
            }
            string_append_string(&temp, string_unpack(&flag->flag));

            compiler_flags_remove_flag(flags, string_unpack(&temp));
            string_free(&temp);
        }
    } else {
        log_fatal("command_compiler: unknown command type");
    }

    config_target_save(data, &cmd->target);
    return 0;
}

bld_target_build_information command_compiler_initial_setup(bld_command_compiler* cmd, bld_data* data) {
    if (!set_has(&data->targets, string_hash(string_unpack(&cmd->target)))) {
        log_fatal("'%s' is not a target", string_unpack(&cmd->target));
    }

    config_target_load(data, &cmd->target);
    if (!data->target_config_parsed) {
        log_fatal("Config for target '%s' could not be parsed", string_unpack(&cmd->target));
    }
    if (!os_file_exists(path_to_string(&cmd->path))) {
        log_fatal("Requested file '%s' does not exist", path_to_string(&cmd->path));
    }

    return utils_index_project(data); /* Unify existing with current */
}

int command_compiler_convert(bld_command* pre_cmd, bld_data* data, bld_command_compiler* cmd, bld_command_invalid* invalid) {
    bld_string err;
    bld_command_positional* arg;
    bld_command_positional_required* path;
    bld_command_positional_optional* target;
    bld_command_positional_optional* option;
    bld_command_positional_optional* compiler;

    if (!data->has_root) {
        err = string_copy(&bld_command_init_missing_project);
        goto parse_failed;
    }

    if (data->targets.size == 0) {
        err = string_copy(&bld_command_init_no_targets);
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 0);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_compiler_convert: missing first optional");}
    target = &arg->as.opt;

    if (!utils_get_target(&cmd->target, &err, target, data)) {
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 2);
    if (arg->type != BLD_HANDLE_POSITIONAL_REQUIRED) {log_fatal("command_compiler_convert: missing required");}
    path = &arg->as.req;

    arg = array_get(&pre_cmd->positional, 3);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_compiler_convert: missing option");}
    option = &arg->as.opt;

    arg = array_get(&pre_cmd->positional, 4);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_compiler_convert: missing compiler");}
    compiler = &arg->as.opt;

    if (option->present) {
        bld_string add, remove, cc, clear;

        add = string_pack("++");
        remove = string_pack("--");
        cc = string_pack("cc");
        clear = string_pack("clear");
        if (!(string_eq(&option->value, &add) || string_eq(&option->value, &remove) || string_eq(&option->value, &cc) || string_eq(&option->value, &clear))) {
            string_free(&cmd->target);
            err = string_new();
            string_append_string(&err, "Expected option to set compiler, add or remove with cc/++/--/clear, got ");
            string_append_string(&err, string_unpack(&option->value));
            string_append_char(&err, '\n');
            goto parse_failed;
        }

        if (string_eq(&option->value, &cc)) {
            cmd->type = BLD_COMMAND_COMPILER_SET_COMPILER;
            if (!compiler->present) {
                string_free(&cmd->target);

                err = string_pack("Missing compiler argument\n");
                err = string_copy(&err);
                goto parse_failed;
            }
            cmd->compiler = string_copy(&compiler->value);
        } else if (string_eq(&option->value, &add)) {
            cmd->type = BLD_COMMAND_COMPILER_ADD_FLAGS;
            if (compiler->present) {
                err = string_new();
                string_append_string(&err, "Unexpected argument '");
                string_append_string(&err, string_unpack(&compiler->value));
                string_append_string(&err, "' when adding flags\n");
                goto parse_failed;
            }
        } else if (string_eq(&option->value, &remove)) {
            cmd->type = BLD_COMMAND_COMPILER_REMOVE_FLAGS;
            if (compiler->present) {
                err = string_new();
                string_append_string(&err, "Unexpected argument '");
                string_append_string(&err, string_unpack(&compiler->value));
                string_append_string(&err, "' when removing flags\n");
                goto parse_failed;
            }
        } else if (string_eq(&option->value, &clear)) {
            cmd->type = BLD_COMMAND_COMPILER_CLEAR;
            if (compiler->present) {
                err = string_new();
                string_append_string(&err, "Unexpected argument '");
                string_append_string(&err, string_unpack(&compiler->value));
                string_append_string(&err, "' when clearing compiler info\n");
                goto parse_failed;
            }
        }

        cmd->flags = pre_cmd->extra_flags;
        pre_cmd->extra_flags = array_new(sizeof(bld_command_flag));
    } else {
        if (compiler->present) {
            log_fatal("Unreachable error, compiler but no option");
        }
        cmd->type = BLD_COMMAND_COMPILER_PRINT;
    }

    cmd->path = path_from_string(string_unpack(&path->value));

    return 0;
    parse_failed:
    *invalid = command_invalid_new(-1, &err);
    return -1;
}

bld_handle_annotated command_handle_compiler(char* name) {
    bld_string temp;
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_COMPILER;
    handle.name = bld_command_string_compiler;
    handle.handle = handle_new(name);
    handle_positional_optional(&handle.handle, "Target to set compiler flags for");
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_compiler));
    handle_positional_required(&handle.handle, "Directory/File to set flags for");
    handle_positional_optional(&handle.handle, "Set compiler with cc or add/remove flags with ++ or --");
    handle_allow_flags(&handle.handle);
    handle_positional_optional(&handle.handle, "The compiler to set if previous argument is cc");
    handle_allow_arbitrary_flags(&handle.handle, "Arbitrary flags can be specified which will be passed to the specified compiler");

    temp = string_new();
    string_append_string(
        &temp,
        "Set or view the compiler flags of a file/directory. A file/directory\n"
        "will inherit the compiler and compiler flags of the directory it is in.\n"
        "When changing the compiler information of a file there are three different\n"
        "modes:\n"
        "\n"
        "    `bld <target> compiler <path> cc <compiler> <flags...>`:\n"
        "        This will set the compiler of the file/directory at the specified\n"
        "        path, that file/directory will then no longer inherit any compiler\n"
        "        information from the parent directory.\n"
    );
    string_append_string(
        &temp,
        "\n"
        "    `bld <target> compiler <path> ++ <flags...>`:\n"
        "        This will add compiler flags to the path, the file/directory at\n"
        "        this path will still inherit from the parent directory.\n"
        "\n"
        "    `bld <target> compiler <path> -- <flags...>`:\n"
        "        This will remove compiler flags from the path, the file/directory\n"
        "        at this path will still inherit the compiler and flags from the\n"
        "        parent directory except for the flags that were removed.\n"
    );
    string_append_string(
        &temp,
        "\n"
        "To see what compiler information a file/directory has run the command\n"
        "\n"
        "    bld <target> compiler <path to file>"
    );

    handle_set_description(&handle.handle, string_unpack(&temp));

    handle.convert = (bld_command_convert*) command_compiler_convert;
    handle.execute = (bld_command_execute*) command_compiler;
    handle.free = (bld_command_free*) command_compiler_free;

    string_free(&temp);
    return handle;
}

void command_compiler_free(bld_command_compiler* cmd) {
    bld_iter iter;
    bld_command_flag* flag;

    string_free(&cmd->target);
    path_free(&cmd->path);

    if (cmd->type == BLD_COMMAND_COMPILER_SET_COMPILER) {
        string_free(&cmd->compiler);
    }

    if (cmd->type != BLD_COMMAND_COMPILER_PRINT) {
        iter = iter_array(&cmd->flags);
        while (iter_next(&iter, (void**) &flag)) {
            string_free(&flag->flag);
        }
        array_free(&cmd->flags);
    }
}
