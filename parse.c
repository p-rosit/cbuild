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
int parse_project_compiler(FILE*, bld_project*);

int parse_graph(FILE*, bld_project*);
int parse_node(FILE*, bld_project*);
int parse_node_file(FILE*, bld_project*);
int parse_file(FILE*, bld_file*);
int parse_file_type(FILE*, bld_file*);
int parse_file_id(FILE*, bld_file*);
int parse_file_hash(FILE*, bld_file*);
int parse_file_name(FILE*, bld_file*);
int parse_file_compiler(FILE*, bld_file*);
int parse_defined_functions(FILE*, bld_project*);
int parse_undefined_functions(FILE*, bld_project*);
int parse_edges(FILE*, bld_project*);

int parse_compiler(FILE*, bld_compiler*);
int parse_compiler_type(FILE*, bld_compiler*);
int parse_compiler_executable(FILE*, bld_compiler*);
int parse_compiler_options(FILE*, bld_compiler*);

int parse_string(FILE*, bld_string*);

int parse_map(FILE*, void*, int, int*, char**, bld_parse_func*);
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
    } else {
        fclose(file);
        log_debug("Found cache file, loading.");
        // parse_cache(cache, &project->root);
    }

    project->cache = cache;
    free_path(&path);
}

void parse_cache(bld_project* cache, bld_path* root) {
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char keys[2][15] = {"compiler", "project_graph"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_project_compiler,
        (bld_parse_func) parse_graph
    };
    bld_path path = copy_path(root);
    FILE* f;

    append_path(&path, &cache->root);
    append_dir(&path, BLD_CACHE_NAME);
    f = fopen(path_to_string(&path), "r");

    amount_parsed = parse_map(f, cache, size, parsed, (char**) keys, funcs);

    if (amount_parsed != size) {
        log_warn("Parse failed, expected all keys to be present in cache: [");
        for (int i = 0; i < size; i++) {
            if (i > 0) {printf(",\n");}
            printf("\"%s\"", keys[i]);
        }
        printf("]\n");
        fclose(f);
        free_path(&path);
        return;
    }

    fclose(f);
    free_path(&path);
}

int parse_graph(FILE* file, bld_project* project) {
    log_fatal("parse_graph: unimplemented");
}

int parse_node(FILE* file, bld_project* project) {
    int amount_parsed;
    int size = 4;
    int parsed[4];
    char keys[4][15] = {"file", "defined", "undefined", "edges"};
    bld_parse_func funcs[4] = {
        (bld_parse_func) parse_node_file,
        (bld_parse_func) parse_defined_functions,
        (bld_parse_func) parse_undefined_functions,
        (bld_parse_func) parse_edges
    };

    amount_parsed = parse_map(file, project, size, parsed, (char**) keys, funcs);

    if (amount_parsed != size) {
        log_warn("Could not parse node, expected all keys to be present: [");
        for (int i = 0; i < size; i++) {
            if (i > 0) {printf(",\n");}
            printf("\"%s\"", keys[i]);
        }
        printf("]\n");
        return -1;
    }

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
        if (parsed[index]) {
            log_warn("Duplicate key \"%s\" encountered", keys[index]);
            goto parse_failed;
        }
        parsed[index] = 1;

        c = next_character(file);
        if (c != ':') {
            log_warn("Expected \':\', got \'%c\'", c);
            goto parse_failed;
        }

        result = parse_funcs[index](file, obj);
        if (result) {goto parse_failed;}

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
