#include "../bit_core/logging.h"
#include "../bit_core/os.h"
#include "../bit_core/iter.h"
#include "utils.h"

const bld_string bld_path_build = STRING_COMPILE_TIME_PACK(".bit");
const bld_string bld_path_target = STRING_COMPILE_TIME_PACK("target");

bld_path data_find_root(void);
bld_set data_find_targets(bld_path*);

bld_data data_extract(void) {
    bld_data data;

    data.root = data_find_root();
    data.targets = data_find_targets(&data.root);

    return data;
}

void data_free(bld_data* data) {
    bld_string* target;
    bld_iter iter;

    path_free(&data->root);

    iter = iter_set(&data->targets);
    while (iter_next(&iter, (void**) &target)) {
        string_free(target);
    }
    set_free(&data->targets);
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

bld_path data_find_root(void) {
    int root_found = 0;
    char cwd[FILENAME_MAX];
    bld_path root;

    os_cwd(cwd, FILENAME_MAX);
    root = path_from_string(cwd);

    do {
        bld_os_dir* dir;
        bld_path temp = path_copy(&root);
        path_append_string(&temp, string_unpack(&bld_path_build));

        dir = os_dir_open(path_to_string(&temp));
        path_free(&temp);
        if (dir != NULL) {
            os_dir_close(dir);
            root_found = 1;
            break;
        }

        if (root.str.size < 1) {break;}
        path_remove_last_string(&root);
    } while (!root_found);

    if (!root_found) {
        path_free(&root);
        root = path_from_string("");
    }

    return root;
}
