#include "path.h"

path new_path() {
    return (path) {
        .str = new_string(),
    };
}

path copy_path(path* path) {
    return (struct path) {
        .str = copy_string(&path->str);
    };
}
