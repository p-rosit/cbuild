#include "../bit_core/os.h"
#include "../bit_core/logging.h"
#include "utils.h"

const bld_string bld_build_path = STRING_COMPILE_TIME_PACK(".bit");
const bld_string bld_target_path = STRING_COMPILE_TIME_PACK("target");

bld_path bld_find_root(void) {
    int root_found = 0;
    char cwd[FILENAME_MAX];
    bld_path root;

    os_cwd(cwd, FILENAME_MAX);
    root = path_from_string(cwd);

    do {
        bld_os_dir* dir;
        bld_path temp = path_copy(&root);
        path_append_string(&temp, string_unpack(&bld_build_path));

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
