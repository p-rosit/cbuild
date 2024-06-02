#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "linker.h"

const bld_string bld_command_string_linker = STRING_COMPILE_TIME_PACK("linker");

bld_target_build_information command_linker_initial_setup(bld_command_linker*, bld_data*);

int command_linker(bld_command_linker* cmd, bld_data* data) {
    bld_target_build_information flags;
    bld_target_build_information* file;

    flags = command_linker_initial_setup(cmd, data);

    file = utils_get_build_info_for(data, &cmd->path);
    if (file == NULL) {
        log_fatal("Requested file '%s' does not exist in the project", path_to_string(&cmd->path));
    }

    if (cmd->type == BLD_COMMAND_LINKER_PRINT) {
        printf("Target:   %s\n", string_unpack(&cmd->target));
        printf("File:     %s\n", path_to_string(&cmd->path));

        if (file->info.linker_set) {
            bld_linker_flags* flags;
            bld_iter iter;
            bld_string* flag;

            flags = &file->info.linker_flags;

            if (flags->flags.size > 0) {
                int first = 1;
                printf("Flags:    [");

                iter = iter_array(&flags->flags);
                while (iter_next(&iter, (void**) &flag)) {
                    if (!first) {
                        printf(", ");
                    }
                    first = 0;
                    printf("%s", string_unpack(flag));
                }
                printf("]\n");
            } else {
                printf("No linker info set\n");
            }
        } else {
            printf("No linker info set\n");
        }

        config_target_build_info_free(&flags);
        return 0;
    }

    config_target_build_info_free(&flags);
    return 0;
}

bld_target_build_information command_linker_initial_setup(bld_command_linker* cmd, bld_data* data) {
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

int command_linker_convert(bld_command* pre_cmd, bld_data* data, bld_command_linker* cmd, bld_command_invalid* invalid) {
    bld_string err;
    bld_command_positional* arg;
    bld_command_positional_required* path;
    bld_command_positional_optional* target;
    bld_command_positional_optional* option;
    bld_command_positional_optional* linker;

    arg = array_get(&pre_cmd->positional, 0);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_linker_convert: missing first optional");}
    target = &arg->as.opt;

    if (!utils_get_target(&cmd->target, &err, target, data)) {
        goto parse_failed;
    }

    arg = array_get(&pre_cmd->positional, 2);
    if (arg->type != BLD_HANDLE_POSITIONAL_REQUIRED) {log_fatal("command_linker_convert: missing required");}
    path = &arg->as.req;

    arg = array_get(&pre_cmd->positional, 3);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_linker_convert: missing option");}
    option = &arg->as.opt;

    arg = array_get(&pre_cmd->positional, 4);
    if (arg->type != BLD_HANDLE_POSITIONAL_OPTIONAL) {log_fatal("command_linker_convert: missing linker");}
    linker = &arg->as.opt;

    if (option->present) {
        bld_string add, ll, clear;

        add = string_pack("++");
        ll = string_pack("ll");
        clear = string_pack("clear");
        if (!(string_eq(&option->value, &add) || string_eq(&option->value, &ll) || string_eq(&option->value, &clear))) {
            string_free(&cmd->target);
            err = string_new();
            string_append_string(&err, "Expected option to set linker, add or remove with ll/++/clear, got ");
            string_append_string(&err, string_unpack(&option->value));
            string_append_char(&err, '\n');
            goto parse_failed;
        }

        if (string_eq(&option->value, &ll)) {
            cmd->type = BLD_COMMAND_LINKER_SET_LINKER;
            if (!linker->present) {
                string_free(&cmd->target);

                err = string_pack("Missing linker argument\n");
                err = string_copy(&err);
                goto parse_failed;
            }
            cmd->linker = string_copy(&linker->value);
        } else if (string_eq(&option->value, &add)) {
            cmd->type = BLD_COMMAND_LINKER_ADD_FLAGS;
            if (linker->present) {
                err = string_new();
                string_append_string(&err, "Unexpected argument '");
                string_append_string(&err, string_unpack(&linker->value));
                string_append_string(&err, "' when adding flags\n");
                goto parse_failed;
            }
        } else if (string_eq(&option->value, &clear)) {
            cmd->type = BLD_COMMAND_LINKER_CLEAR;
            if (linker->present) {
                err = string_new();
                string_append_string(&err, "Unexpected argument '");
                string_append_string(&err, string_unpack(&linker->value));
                string_append_string(&err, "' when clearing compiler info\n");
                goto parse_failed;
            }
        }

        cmd->flags = pre_cmd->extra_flags;
        pre_cmd->extra_flags = array_new(sizeof(bld_command_flag));
    } else {
        if (linker->present) {
            log_fatal("Unreachable error, compiler but no option");
        }
        cmd->type = BLD_COMMAND_LINKER_PRINT;
    }

    cmd->path = path_from_string(string_unpack(&path->value));

    return 0;
    parse_failed:
    *invalid = command_invalid_new(-1, &err);
    return -1;
}

bld_handle_annotated command_handle_linker(char* name) {
    bld_handle_annotated handle;

    handle.type = BLD_COMMAND_LINKER;
    handle.handle = handle_new(name);
    handle_positional_optional(&handle.handle, "Target to set linker for");
    handle_positional_expect(&handle.handle, string_unpack(&bld_command_string_linker));
    handle_positional_required(&handle.handle, "Directory/File to set flags for");
    handle_positional_optional(&handle.handle, "Set linker with ll or add flags with ++");
    handle_allow_flags(&handle.handle);
    handle_positional_optional(&handle.handle, "The linker to set if the previous argument is ll");
    handle_allow_arbitrary_flags(&handle.handle, "Passthrough linker flags");
    handle_set_description(&handle.handle, "Set linker flags");

    handle.convert = (bld_command_convert*) command_linker_convert;
    handle.execute = (bld_command_execute*) command_linker;
    handle.free = (bld_command_free*) command_linker_free;

    return handle;
}

void command_linker_free(bld_command_linker* cmd) {
    string_free(&cmd->target);
    path_free(&cmd->path);

    if (cmd->type == BLD_COMMAND_LINKER_SET_LINKER) {
        string_free(&cmd->linker);
    }

    if (cmd->type != BLD_COMMAND_LINKER_PRINT) {
        bld_command_flag* flag;
        bld_iter iter = iter_array(&cmd->flags);

        while (iter_next(&iter, (void**) &flag)) {
            string_free(&flag->flag);
        }
        array_free(&cmd->flags);
    }
}
