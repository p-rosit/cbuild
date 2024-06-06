#include <string.h>
#include "../bld_core/logging.h"
#include "../bld_core/os.h"
#include "../bld_core/iter.h"
#include "utils.h"
#include "add.h"
#include "ignore.h"
#include "init.h"
#include "help.h"
#include "compiler.h"
#include "linker.h"
#include "switch.h"
#include "invalidate.h"
#include "remove.h"
#include "status.h"
#include "build.h"
#include "invalid.h"

const bld_string bld_path_build = STRING_COMPILE_TIME_PACK(".bld");
const bld_string bld_path_target = STRING_COMPILE_TIME_PACK("target");
const bld_string bld_path_config = STRING_COMPILE_TIME_PACK("config.json");

int data_find_root(bld_path*);
bld_set data_find_targets(bld_path*);
void data_add_handle(bld_data*, bld_handle_annotated);
bld_target_build_information* utils_get_build_info_recursive(bld_path*, bld_target_build_information*, uintmax_t);
void utils_apply_build_information_recursive(bld_target_build_information*, bld_target_build_information*);

void config_load(bld_data* data) {
    int error;
    bld_path target_path = path_copy(&data->root);
    path_append_string(&target_path, string_unpack(&bld_path_build));
    path_append_string(&target_path, string_unpack(&bld_path_config));

    if (data->config_parsed) {log_fatal("config_load: config already parsed");}
    error = parse_config(&target_path, &data->config);
    data->config_parsed = !error;

    path_free(&target_path);
}

void config_save(bld_data* data) {
    bld_path target_path = path_copy(&data->root);
    path_append_string(&target_path, string_unpack(&bld_path_build));
    path_append_string(&target_path, string_unpack(&bld_path_config));

    serialize_config(&target_path, &data->config);
    path_free(&target_path);
}

void config_target_load(bld_data* data, bld_string* target) {
    int error;
    bld_path target_path = path_copy(&data->root);
    path_append_string(&target_path, string_unpack(&bld_path_build));
    path_append_string(&target_path, string_unpack(&bld_path_target));
    path_append_string(&target_path, string_unpack(target));
    path_append_string(&target_path, string_unpack(&bld_path_config));

    if (data->target_config_parsed) {log_fatal("config_target_load: config already parsed");}
    error = parse_config_target(&target_path, &data->target_config);
    data->target_config_parsed = !error;

    path_free(&target_path);
}

void config_target_save(bld_data* data, bld_string* target) {
    bld_path target_path = path_copy(&data->root);
    path_append_string(&target_path, string_unpack(&bld_path_build));
    path_append_string(&target_path, string_unpack(&bld_path_target));
    path_append_string(&target_path, string_unpack(target));
    path_append_string(&target_path, string_unpack(&bld_path_config));

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
        path_append_string(&path_config, string_unpack(&bld_path_build));
        path_append_string(&path_config, string_unpack(&bld_path_config));

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
    data_add_handle(&data, command_handle_linker(name));
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

bld_target_build_information utils_index_project_recursive_file(bld_path*, bld_config_target*);
bld_target_build_information utils_index_project_recursive(bld_path*, bld_config_target*, bld_set*);

bld_target_build_information utils_index_project(bld_data* data) {
    bld_iter iter;
    bld_path* ignored;
    bld_target_build_information info;
    bld_string name;
    bld_set ignored_ids;

    if (!data->target_config_parsed) {
        log_fatal("utils_index_project: target config has not been parsed");
    }

    ignored_ids = set_new(0);
    iter = iter_array(&data->target_config.ignore_paths);
    while (iter_next(&iter, (void**) &ignored)) {
        bld_path temp = path_copy(&data->root);
        path_append_path(&temp, ignored);
        set_add(&ignored_ids, os_info_id(path_to_string(&temp)), NULL);
        path_free(&temp);
    }

    info = utils_index_project_recursive(&data->root, &data->target_config, &ignored_ids);
    name = string_pack(".");
    info.name = string_copy(&name);

    set_free(&ignored_ids);
    return info;
}

bld_target_build_information utils_index_project_recursive(bld_path* path, bld_config_target* config, bld_set* ignored_ids) {
    bld_path root;
    bld_iter iter;
    bld_os_dir* dir;
    bld_string name;
    bld_target_build_information info;
    (void)(config);

    dir = os_dir_open(path_to_string(path));
    if (dir == NULL) {
        return utils_index_project_recursive_file(path, config);
    }

    info.info.compiler_set = 0;
    info.info.linker_set = 0;
    info.files = array_new(sizeof(bld_target_build_information));

    root = path_copy(path);
    iter = iter_directory(dir);
    while (iter_next(&iter, (void**) &name)) {
        bld_path entry_path;
        bld_target_build_information entry_info;
        if (*name.chars == '.') {continue;}
        entry_path = path_copy(&root);
        path_append_string(&entry_path, string_unpack(&name));

        if (set_has(ignored_ids, os_info_id(path_to_string(&entry_path)))) {
            path_free(&entry_path);
            continue;
        }

        {
            char* file_ending = strrchr(string_unpack(&name), '.');
            if (file_ending == NULL) {
                if (!os_dir_exists(path_to_string(&entry_path))) {
                    path_free(&entry_path);
                    continue;
                }
            } else if (compiler_file_is_implementation(&config->compiler_handles, &name)) {
            } else if (compiler_file_is_header(&config->compiler_handles, &name)) {
            } else {
                path_free(&entry_path);
                continue;
            }
        }

        entry_info = utils_index_project_recursive(&entry_path, config, ignored_ids);
        entry_info.name = string_copy(&name);
        array_push(&info.files, &entry_info);

        path_free(&entry_path);
    }

    path_free(&root);
    os_dir_close(dir);
    return info;
}

bld_target_build_information utils_index_project_recursive_file(bld_path* path, bld_config_target* data) {
    bld_target_build_information info;
    info.info.compiler_set = 0;
    info.info.linker_set = 0;
    info.files = array_new(sizeof(bld_target_build_information));
    (void)(path);
    (void)(data);
    return info;
}

bld_target_build_information* utils_get_build_info_for(bld_data* data, bld_path* path) {
    uintmax_t requested_id = os_info_id(path_to_string(path));

    if (!data->has_root) {log_fatal("No root, no init?");}
    if (!data->target_config_parsed) {log_fatal("No target config parsed");}
    if (requested_id == BLD_INVALID_IDENITIFIER) {return NULL;}

    return utils_get_build_info_recursive(&data->root, &data->target_config.files, requested_id);
}

bld_target_build_information* utils_get_build_info_recursive(bld_path* path, bld_target_build_information* root, uintmax_t requested_id) {
    bld_iter iter;
    bld_target_build_information* file;
    bld_path current_path;
    uintmax_t current_id;

    current_path = path_copy(path);
    path_append_string(&current_path, string_unpack(&root->name));
    current_id = os_info_id(path_to_string(&current_path));

    if (current_id == requested_id) {
        path_free(&current_path);
        return root;
    }

    iter = iter_array(&root->files);
    while (iter_next(&iter, (void**) &file)) {
        bld_target_build_information* result;
        result = utils_get_build_info_recursive(&current_path, file, requested_id);
        if (result != NULL) {
            path_free(&current_path);
            return result;
        }
    }

    path_free(&current_path);
    return NULL;
}

void utils_apply_build_information(bld_data* data, bld_target_build_information* info) {
    if (!data->target_config_parsed) {
        log_fatal("utils_apply_build_information: target config not parsed");
    }

    utils_apply_build_information_recursive(info, &data->target_config.files);
    string_free(&data->target_config.files.name);
    data->target_config.files = *info;
}

void utils_apply_build_information_recursive(bld_target_build_information* project, bld_target_build_information* info) {
    bld_iter iter;
    bld_target_build_information* child;
    project->info = info->info;

    iter = iter_array(&info->files);
    while (iter_next(&iter, (void**) &child)) {
        int exists = 0;
        bld_iter iter;
        bld_target_build_information* corresponding;

        iter = iter_array(&project->files);
        while (iter_next(&iter, (void**) &corresponding)) {
            if (string_eq(&child->name, &corresponding->name)) {
                exists = 1;
                break;
            }
        }

        if (!exists) {
            config_target_build_info_free(child);
            continue;
        }

        utils_apply_build_information_recursive(corresponding, child);
        string_free(&child->name);
    }

    array_free(&info->files);
}
