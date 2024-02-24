#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include "build.h"
#include "path.h"
#include "project.h"

typedef int (*bld_parse_func)(FILE*, void*);

bld_project make_project(bld_path, bld_compiler);
void parse_cache(bld_project*, bld_path*);
int parse_compiler(FILE*, bld_compiler*);
int parse_string(FILE*, bld_string*);

int next_character(FILE*);

void ensure_directory_exists(bld_path* directory_path) {
    errno = 0;
    mkdir(path_to_string(directory_path), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    switch (errno) {
        case 0: {
            log_debug("Creating cache directory.");
        } break;
        case EACCES: {
            log_fatal("No access to create cache directory.");
        } break;
        case EEXIST: {
            log_debug("Found cache directory.");
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

void load_cache(bld_project* project, char* cache_path) {
    FILE* file;
    bld_path path, temp;
    bld_project* cache;

    path = copy_path(&project->root);
    append_dir(&path, cache_path);
    ensure_directory_exists(&path);

    append_dir(&path, BLD_CACHE_NAME);
    file = fopen(path_to_string(&path), "r");

    cache = malloc(sizeof(bld_project));
    if (cache == NULL) {log_fatal("Could not allocate cache.");}
    temp = path_from_string(cache_path);
    *cache = make_project(temp, new_compiler(BLD_INVALID_COMPILER, ""));

    if (file == NULL) {
        log_debug("No cache file found.");
        // log_fatal("load_cache: unimplemented path");
    } else {
        fclose(file);
        log_debug("Found cache file, loading.");
        parse_cache(cache, &project->root);
    }

    project->cache = cache;
    free_path(&path);
}

void parse_cache(bld_project* cache, bld_path* root) {
    size_t key_num;
    int result, parse_complete, parsed_compiler, parsed_graph;
    bld_path path = copy_path(root);
    FILE* f;
    bld_string str;
    char c;

    append_path(&path, &cache->root);
    append_dir(&path, BLD_CACHE_NAME);
    f = fopen(path_to_string(&path), "r");

    log_warn("parse_cache: unimplemented");

    fclose(f);
    free_path(&path);
}

int parse_compiler(FILE* file, bld_compiler* compiler) {
    return 0;
}

int parse_string(FILE* file, bld_string* str) {
    return 0;
}

int next_character(FILE* file) {
    int c = getc(file);
    while (c != EOF && isspace(c)) {getc(file);}
    return c;
}

int parse_map(FILE* file, void* obj, int entries, int* parsed, char** keys, bld_parse_func* parse_funcs) {
    int exists, index, key_num = 0;
    int result, parse_complete;
    bld_string str = new_string();
    char c, *temp;

    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c != '{') {log_warn("Unexpected starting character: \'%c\'", c); goto parse_failed;}

    while (!parse_complete) {
        key_num += 1;
        str = new_string();
        result = parse_string(file, &str);
        if (result) {
            log_warn("Key %d could not be parsed, expected: [", key_num);
            for (int i = 0; i < entries; i++) {
                if (i > 0) {printf(",\n");}
                printf("  \"%s\"", keys[i]);
            }
            printf("]");
            goto parse_failed;
        }

        exists = 0;
        temp = make_string(&str);
        for (int i = 0; i < entries; i++) {
            if (strcmp(temp, keys[i]) == 0) {
                exists = 1;
                index = i;
            }
        }

        if (!exists) {
            log_warn("\"%s\" is not a valid key, expected: [", temp);
            for (int i = 0; i < entries; i++) {
                if (i > 0) {printf(",\n");}
                printf("  \"%s\"", keys[i]);
            }
            printf("]");
            goto parse_failed;
        }

        c = next_character(file);
        if (c != ':') {
            log_warn("Expected \':\', got \'%c\'", c);
            goto parse_failed;
        }

        parse_funcs[index](file, obj);

        free_string(&str);
        str = new_string();
        c = next_character(file);
        switch (c) {
            case ('}'): {
                parse_complete = 1;
            } break;
            case (','): {
                continue;
            } break;
            case (EOF): {
                log_warn("Unexpected EOF");
                goto parse_failed;
            } break;
            default: {
                log_warn("Unexpected character, got \'%c\'", c);
            } break;
        }
    }

    return key_num;
    parse_failed:
    free_string(&str);
    return -1;
}
