#include "../bld_core/logging.h"
#include "../bld_core/os.h"
#include "../bld_core/iter.h"
#include "utils.h"

const bld_string bld_path_build = STRING_COMPILE_TIME_PACK(".bld");
const bld_string bld_path_target = STRING_COMPILE_TIME_PACK("target");

int data_find_root(bld_path*);
bld_set data_find_targets(bld_path*);

bld_data data_extract(void) {
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

    return data;
}

void data_free(bld_data* data) {
    bld_string* target;
    bld_iter iter;

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
