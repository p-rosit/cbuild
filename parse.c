#include <errno.h>
#include <sys/stat.h>
#include "build.h"
#include "path.h"
#include "project.h"

void ensure_directory_exists(bld_path* directory_path) {
    errno = 0;
    mkdir(path_to_string(directory_path), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    switch (errno) {
        case 0: {
            log_info("Creating cache directory.");
        } break;
        case EACCES: {
            log_fatal("No access to create cache directory.");
        } break;
        case EEXIST: {
            log_info("Found cache directory.");
        } break;
        case ENAMETOOLONG: {
            log_fatal("Name of cache directory too long.");
        } break;
        case EROFS: {
            log_fatal("Cache directory cannot be created in a read-only file system.");
        } break;
        default: {log_fatal("load_cache: unreachable error???");}
    }
}

void load_cache(bld_project project, char* cache_path) {
    FILE* file;
    bld_path path;
    bld_cache* cache;

    path = copy_path(&project.root);
    append_dir(&path, cache_path);
    ensure_directory_exists(&path);

    append_dir(&path, "cache.json");
    file = fopen(path_to_string(&path), "r");
    if (file == NULL) {
        log_info("No cache file found.");
    } else {
        log_info("Found cache file, loading.");
        cache = malloc(sizeof(bld_cache));
        if (cache == NULL) {log_fatal("Could not allocate cache.");}

        *cache = parse_cache(file);
        *(project.cache) = cache;
        fclose(file);
    }
    free_path(&path);
}
