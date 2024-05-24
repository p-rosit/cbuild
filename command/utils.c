#include "../bld_core/logging.h"
#include "../bld_core/os.h"
#include "../bld_core/iter.h"
#include "utils.h"
#include "add.h"
#include "ignore.h"
#include "init.h"
#include "help.h"
#include "compiler.h"
#include "switch.h"
#include "invalidate.h"
#include "remove.h"
#include "status.h"
#include "build.h"
#include "invalid.h"

const bld_string bld_path_build = STRING_COMPILE_TIME_PACK(".bld");
const bld_string bld_path_target = STRING_COMPILE_TIME_PACK("target");

int data_find_root(bld_path*);
bld_set data_find_targets(bld_path*);
void data_add_handle(bld_data*, bld_handle_annotated);

void config_load(bld_data* data) {
    int error;
    bld_path target_path = path_copy(&data->root);
    path_append_string(&target_path, ".bld");
    path_append_string(&target_path, "config.json");

    if (data->config_parsed) {log_fatal("config_load: config already parsed");}
    error = parse_config(&target_path, &data->config);
    data->config_parsed = !error;

    path_free(&target_path);
}

void config_save(bld_data* data) {
    bld_path target_path = path_copy(&data->root);
    path_append_string(&target_path, ".bld");
    path_append_string(&target_path, "config.json");

    serialize_config(&target_path, &data->config);
    path_free(&target_path);
}

void config_target_load(bld_data* data, bld_string* target) {
    int error;
    bld_path target_path = path_copy(&data->root);
    path_append_string(&target_path, ".bld");
    path_append_string(&target_path, "target");
    path_append_string(&target_path, string_unpack(target));
    path_append_string(&target_path, "config.json");

    if (data->target_config_parsed) {log_fatal("config_target_load: config already parsed");}
    error = parse_config_target(&target_path, &data->target_config);
    data->target_config_parsed = !error;

    path_free(&target_path);
}

void config_target_save(bld_data* data, bld_string* target) {
    bld_path target_path = path_copy(&data->root);
    path_append_string(&target_path, ".bld");
    path_append_string(&target_path, "target");
    path_append_string(&target_path, string_unpack(target));
    path_append_string(&target_path, "config.json");

    serialize_config_target(&target_path, &data->target_config);
    path_free(&target_path);
}

bld_data data_extract(char* name) {
    bld_data data;

    data.has_root = data_find_root(&data.root);
    if (data.has_root) {
        data.targets = data_find_targets(&data.root);
    } else {
        data.targets = set_new(sizeof(bld_string));
    }

    data.config_parsed = 0;
    data.target_config_parsed = 0;

    if (data.has_root) {
        bld_path path_config = path_copy(&data.root);
        path_append_string(&path_config, ".bld");
        path_append_string(&path_config, "config.json");

        data.config_parsed = !parse_config(&path_config, &data.config);
        path_free(&path_config);
    }

    data.handles = set_new(sizeof(bld_handle_annotated));
    data.handle_order = array_new(sizeof(bld_command_type));
    data_add_handle(&data, command_handle_help(name));
    data_add_handle(&data, command_handle_add(name));
    data_add_handle(&data, command_handle_ignore(name));
    data_add_handle(&data, command_handle_switch(name));
    data_add_handle(&data, command_handle_compiler(name));
    data_add_handle(&data, command_handle_remove(name));
    data_add_handle(&data, command_handle_invalidate(name));
    data_add_handle(&data, command_handle_status(name));
    data_add_handle(&data, command_handle_init(name));
    data_add_handle(&data, command_handle_build(name));
    data_add_handle(&data, command_handle_invalid(name));

    return data;
}

void data_free(bld_data* data) {
    bld_iter iter;
    bld_string* target;
    bld_handle_annotated* handle;

    if (data->has_root) {
        path_free(&data->root);
    }

    iter = iter_set(&data->targets);
    while (iter_next(&iter, (void**) &target)) {
        string_free(target);
    }
    set_free(&data->targets);

    if (data->config_parsed) {
        config_free(&data->config);
    }

    if (data->target_config_parsed) {
        config_target_free(&data->target_config);
    }

    iter = iter_set(&data->handles);
    while (iter_next(&iter, (void**) &handle)) {
        if (handle->type == BLD_COMMAND_INVALID) {continue;}
        handle_free(&handle->handle);
    }
    set_free(&data->handles);
    array_free(&data->handle_order);
}

bld_set data_find_targets(bld_path* root) {
    char* file_name;
    bld_path cpy;
    bld_set targets;
    bld_os_dir* dir;
    bld_os_file* file;

    cpy = path_copy(root);
    path_append_string(&cpy, string_unpack(&bld_path_build));
    path_append_string(&cpy, string_unpack(&bld_path_target));

    targets = set_new(sizeof(bld_string));
    dir = os_dir_open(path_to_string(&cpy));
    if (dir == NULL) {
        path_free(&cpy);
        return targets;
    }

    while ((file = os_dir_read(dir)) != NULL) {
        bld_string name;

        file_name = os_file_name(file);
        if (file_name[0] == '.') {continue;}

        name = string_pack(file_name);
        name = string_copy(&name);

        set_add(&targets, string_hash(file_name), &name);
    }

    os_dir_close(dir);
    path_free(&cpy);
    return targets;
}

int data_find_root(bld_path* root) {
    int root_found = 0;
    char cwd[FILENAME_MAX];
    bld_path test_root;

    os_cwd(cwd, FILENAME_MAX);
    test_root = path_from_string(cwd);

    do {
        bld_os_dir* dir;
        bld_path temp = path_copy(&test_root);
        path_append_string(&temp, string_unpack(&bld_path_build));

        dir = os_dir_open(path_to_string(&temp));
        path_free(&temp);
        if (dir != NULL) {
            os_dir_close(dir);
            root_found = 1;
            break;
        }

        if (test_root.str.size < 1) {break;}
        path_remove_last_string(&test_root);
    } while (!root_found);

    if (root_found) {
        *root = test_root;
        return 1;
    } else {
        path_free(&test_root);
        return 0;
    }
}

void data_add_handle(bld_data* data, bld_handle_annotated handle) {
    set_add(&data->handles, handle.type, &handle);
    array_push(&data->handle_order, &handle.type);
}

int utils_get_target(bld_string* target, bld_string* err, bld_command_positional_optional* arg, bld_data* data) {
    if (arg->present) {
        *target = string_copy(&arg->value);
    } else if (data->config_parsed) {
        if (data->config.active_target_configured) {
            if (!set_has(&data->targets, string_hash(string_unpack(&data->config.active_target)))) {
                *err = string_new();
                string_append_string(err, "config has active target ");
                string_append_string(err, string_unpack(&data->config.active_target));
                string_append_string(err, " which is not a known target\n");
                return 0;
            }
            *target = string_copy(&data->config.active_target);
        } else {
            *err = string_pack("no target specified and no default target set up\n");
            *err = string_copy(err);
            return 0;
        }
    }

    return 1;
}
