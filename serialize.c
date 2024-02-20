#include <errno.h>
#include "build.h"
#include "project.h"

void save_cache(bld_project project) {
    bld_path cache_path, file_path;

    if (project.cache == NULL) {
        log_warn("Trying to save cache but no cache was loaded. Ignoring.");
        return;
    }

    errno = 0;
    log_warn("save_cache: not implemented.");
}
