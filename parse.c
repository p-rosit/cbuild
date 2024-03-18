#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <string.h>
#include "logging.h"
#include "path.h"
#include "project.h"

typedef int (*bld_parse_func)(FILE*, void*);

bld_project make_project(bld_path, bld_compiler);
int parse_cache(bld_project*, bld_path*);
int parse_project_compiler(FILE*, bld_project*);

int parse_project_files(FILE*, bld_project*);
int parse_file(FILE*, bld_files*);
int parse_file_type(FILE*, bld_file*);
int parse_file_id(FILE*, bld_file*);
int parse_file_hash(FILE*, bld_file*);
int parse_file_name(FILE*, bld_file*);
int parse_file_compiler(FILE*, bld_file*);
int parse_file_defined_symbols(FILE*, bld_file*);
int parse_file_undefined_symbols(FILE*, bld_file*);
int parse_file_function(FILE*, bld_set*);
int parse_file_includes(FILE*, bld_file*);
int parse_file_include(FILE*, bld_set*);

int parse_project_graph(FILE*, bld_project*);
int parse_graph_node(FILE*, bld_project*);
int parse_node_file(FILE*, bld_node*);
int parse_node_symbol_edges(FILE*, bld_node*);
int parse_node_include_edges(FILE*, bld_node*);
int parse_node_edge(FILE*, bld_array*);

int parse_compiler(FILE*, bld_compiler*);
int parse_compiler_type(FILE*, bld_compiler*);
int parse_compiler_executable(FILE*, bld_compiler*);
int parse_compiler_options(FILE*, bld_compiler*);
int parse_compiler_option(FILE*, bld_options*);

int parse_string(FILE*, bld_string*);
int parse_uintmax(FILE*, uintmax_t*);
int parse_uintmax_array(FILE*, bld_array*);
int parse_uintmax_set(FILE*, bld_set*);

int parse_array(FILE*, void*, bld_parse_func);
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
        default: {perror("Unreachable error?"); log_fatal("load_cache: unreachable error???");}
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
        int result;

        fclose(file);
        log_debug("Found cache file, attempting to parse.");
        free_compiler(&cache->compiler);
        result = parse_cache(cache, &project->root);

        if (result) {
            log_warn("Could not parse cache, ignoring.");
        } else {
            log_info("Loaded cache.");
        }
    }

    project->cache = cache;
    free_path(&path);
}

int parse_cache(bld_project* cache, bld_path* root) {
    int amount_parsed;
    int size = 3;
    int parsed[3];
    char *keys[3] = {"compiler", "files", "graph"};
    bld_parse_func funcs[3] = {
        (bld_parse_func) parse_project_compiler,
        (bld_parse_func) parse_project_files,
        (bld_parse_func) parse_project_graph,
    };
    bld_path path = copy_path(root);
    FILE* f;

    append_path(&path, &cache->root);
    append_dir(&path, BLD_CACHE_NAME);
    f = fopen(path_to_string(&path), "r");

    amount_parsed = parse_map(f, cache, size, parsed, keys, funcs);

    if (amount_parsed != size) {
        fclose(f);
        free_path(&path);
        return -1;
    }

    cache->graph.files = &cache->files;

    fclose(f);
    free_path(&path);
    return 0;
}

int parse_project_compiler(FILE* file, bld_project* project) {
    int result = parse_compiler(file, &project->compiler);
    if (result) {log_warn("Could not parse compiler for project.");}
    return result;
}

int parse_project_files(FILE* file, bld_project* project) {
    int amount_parsed;
    bld_files* files = &project->files;

    project->files = new_files();
    amount_parsed = parse_array(file, files, (bld_parse_func) parse_file);
    if (amount_parsed < 0) {
        log_warn("Could not parse graph");
        return -1;
    }

    return 0;
}

int parse_file(FILE* file, bld_files* files) {
    bld_file temp, *f;
    int amount_parsed;
    int size = 8;
    int parsed[8];
    char *keys[8] = {"type", "id", "hash", "name", "compiler", "includes", "defined_symbols", "undefined_symbols"};
    bld_parse_func funcs[8] = {
        (bld_parse_func) parse_file_type,
        (bld_parse_func) parse_file_id,
        (bld_parse_func) parse_file_hash,
        (bld_parse_func) parse_file_name,
        (bld_parse_func) parse_file_compiler,
        (bld_parse_func) parse_file_includes,
        (bld_parse_func) parse_file_defined_symbols,
        (bld_parse_func) parse_file_undefined_symbols,
    };

    temp.defined_symbols = bld_set_new(sizeof(char*));
    temp.undefined_symbols = bld_set_new(sizeof(char*));

    f = &temp;
    f->compiler = NULL;
    amount_parsed = parse_map(file, f, size, parsed, keys, funcs);
    if (amount_parsed < 0) {return -1;}
    if (!parsed[0]) {return -1;} /* Fail if file type was not parsed */

    if (f->type == BLD_HEADER) {
        /* Header must have: id, hash, name, includes */
        if (!parsed[1] || !parsed[2] || !parsed[3] || !parsed[5]) {
            log_warn("Header did not have required fields");
            return -1;
        }
        /* Header cannot have: defined_symbols, undefined_symbols */
        if (parsed[6] || parsed[7]) {
            log_warn("Header had forbidden fields");
            return -1;
        }
    } else {
        /* File must have every key except an optional compiler */
        if ((amount_parsed < size - 1) || ((amount_parsed == size - 1) && parsed[4])) {
            log_warn("File is missing required field");
            return -1;
        }
    }

    f->path = new_path();
    bld_set_add(&files->set, f->identifier.id, f);
    return 0;
}

int parse_file_type(FILE* file, bld_file* f) {
    bld_string str = string_new();
    char* temp;
    int result = parse_string(file, &str);

    if (result) {
        string_free(&str);
        log_warn("Could not parse compiler type");
        return -1;
    }
    
    temp = string_unpack(&str);
    if (strcmp(temp, "implementation") == 0) {
        f->type = BLD_IMPL;
    } else if (strcmp(temp, "header") == 0) {
        f->type = BLD_HEADER;
    } else if (strcmp(temp, "test") == 0) {
        f->type = BLD_TEST;
    } else {
        log_warn("Not a valid file type: \"%s\"", temp);
        result = -1;
    }

    string_free(&str);
    return result;
}

int parse_file_id(FILE* file, bld_file* f) {
    uintmax_t num;
    int result = parse_uintmax(file, &num);
    if (result) {
        log_warn("Could not parse file id");
        return -1;
    }
    f->identifier.id = num;
    return 0;
}

int parse_file_hash(FILE* file, bld_file* f) {
    uintmax_t num;
    int result = parse_uintmax(file, &num);
    if (result) {
        log_warn("Could not parse file id");
        return -1;
    }
    f->identifier.hash = num;
    return 0;
}

int parse_file_name(FILE* file, bld_file* f) {
    bld_string str = string_new();
    int result = parse_string(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse file name");
        return -1;
    }
    
    f->name = str;
    return result;
}

int parse_file_compiler(FILE* file, bld_file* f) {
    int result;
    f->compiler = malloc(sizeof(bld_compiler));
    if (f->compiler == NULL) {log_fatal("Could not allocate compiler for file");}

    result = parse_compiler(file, f->compiler);
    if (result) {log_warn("Could not parse compiler for file.");}
    return result;
}

int parse_file_includes(FILE* file, bld_file* f) {
    int amount_parsed;

    f->includes = bld_set_new(0);
    amount_parsed = parse_array(file, &f->includes, (bld_parse_func) parse_file_include);
    if (amount_parsed < 0) {
        log_warn("Could not parse file includes");
        return -1;
    }

    return 0;
}

int parse_file_include(FILE* file, bld_set* set) {
    uintmax_t file_id;
    int result;

    result = parse_uintmax(file, &file_id);
    if (result) {return -1;}

    bld_set_add(set, file_id, NULL);
    return 0;
}

int parse_file_defined_symbols(FILE* file, bld_file* f) {
    int amount_parsed;

    amount_parsed = parse_array(file, &f->defined_symbols, (bld_parse_func) parse_file_function);
    if (amount_parsed < 0) {
        log_warn("Could not parse defined symbols");
        return -1;
    }

    return 0;
}

int parse_file_undefined_symbols(FILE* file, bld_file* f) {
    int amount_parsed;

    amount_parsed = parse_array(file, &f->undefined_symbols, (bld_parse_func) parse_file_function);
    if (amount_parsed < 0) {
        log_warn("Could not parse defined symbols");
        return -1;
    }

    return 0;
}

int parse_file_function(FILE* file, bld_set* set) {
    bld_string str = string_new();
    char* temp;
    int result;

    result = parse_string(file, &str);
    if (result) {return -1;}

    temp = string_unpack(&str);
    bld_set_add(set, string_hash(temp, 0), &temp);
    return 0;
}

int parse_project_graph(FILE* file, bld_project* project) {
    int amount_parsed;

    project->graph = new_graph(&project->files); 
    amount_parsed = parse_array(file, project, (bld_parse_func) parse_graph_node);
    if (amount_parsed < 0) {
        log_warn("Could not parse graph");
        return -1;
    }

    return 0;
}

void push_node(bld_nodes*, bld_node);
int parse_graph_node(FILE* file, bld_project* project) {
    int amount_parsed;
    int size = 3;
    int parsed[3];
    char *keys[3] = {"file", "symbol_edges", "include_edges"};
    bld_parse_func funcs[3] = {
        (bld_parse_func) parse_node_file,
        (bld_parse_func) parse_node_symbol_edges,
        (bld_parse_func) parse_node_include_edges,
    };
    bld_nodes* nodes = &project->graph.nodes;
    bld_node node;

    amount_parsed = parse_map(file, &node, size, parsed, keys, funcs);

    if (amount_parsed != size) {
        log_warn("Could not parse node, expected all keys to be present: [");
        for (int i = 0; i < size; i++) {
            if (i > 0) {printf(",\n");}
            printf("  \"%s\"", keys[i]);
        }
        printf("\n]\n");
        return -1;
    }

    push_node(nodes, node);

    return 0;
}

int parse_node_file(FILE* file, bld_node* node) {
    uintmax_t file_id;
    int result;

    result = parse_uintmax(file, &file_id);

    if (result) {
        log_fatal("Free correctly");
        log_warn("Could not parse file of node.");
        return result;
    }

    node->file_id = file_id;
    return 0;
}

int parse_node_symbol_edges(FILE* file, bld_node* node) {
    int amount_parsed;

    node->functions_from.array = bld_array_new(sizeof(uintmax_t));
    amount_parsed = parse_array(file, &node->functions_from.array, (bld_parse_func) parse_node_edge);
    if (amount_parsed < 0) {
        log_warn("Could not parse graph");
        return -1;
    }

    return 0;
}

int parse_node_include_edges(FILE* file, bld_node* node) {
    int amount_parsed;

    node->included_in.array = bld_array_new(sizeof(uintmax_t));
    amount_parsed = parse_array(file, &node->included_in.array, (bld_parse_func) parse_node_edge);
    if (amount_parsed < 0) {
        log_warn("Could not parse graph");
        return -1;
    }

    return 0;
}

int parse_node_edge(FILE* file, bld_array* array) {
    uintmax_t file_id;
    int result;

    result = parse_uintmax(file, &file_id);
    if (result) {return -1;}

    bld_array_push(array, &file_id);
    return 0;
}

int parse_compiler(FILE* file, bld_compiler* compiler) {
    int amount_parsed;
    int size = 3;
    int parsed[3];
    char *keys[3] = {"type", "executable", "options"};
    bld_parse_func funcs[3] = {
        (bld_parse_func) parse_compiler_type,
        (bld_parse_func) parse_compiler_executable,
        (bld_parse_func) parse_compiler_options
    };

    compiler->options = new_options();
    amount_parsed = parse_map(file, compiler, size, parsed, (char**) keys, funcs);

    if (amount_parsed < size && parsed[2]) {
        return -1;
    }
    return 0;
}

int parse_compiler_type(FILE* file, bld_compiler* compiler) {
    bld_string str = string_new();
    char* temp;
    int result = parse_string(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse compiler type");
        return -1;
    }
    
    temp = string_unpack(&str);
    if (strcmp(temp, "gcc") == 0) {
        compiler->type = BLD_GCC;
    } else if (strcmp(temp, "clang") == 0) {
        compiler->type = BLD_CLANG;
    } else {
        log_warn("Not a valid compiler type: \"%s\"", temp);
        result = -1;
    }

    string_free(&str);
    return result;
}

int parse_compiler_executable(FILE* file, bld_compiler* compiler) {
    bld_string str = string_new();
    char* temp;
    int result = parse_string(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse compiler executable");
        return -1;
    }
    
    temp = string_unpack(&str);
    compiler->executable = temp;
    return result;
}

int parse_compiler_options(FILE* file, bld_compiler* compiler) {
    int values;
    bld_options options = new_options();

    values = parse_array(file, &options, (bld_parse_func) parse_compiler_option);
    if (values < 0) {
        log_warn("Could not parse compiler options");
        return -1;
    }
    compiler->options = options;
    return 0;
}

void append_option(bld_options*, char*);
int parse_compiler_option(FILE* file, bld_options* options) {
    bld_string str = string_new();
    char* temp;
    int result = parse_string(file, &str);
    if (result) {
        string_free(&str);
        log_warn("Could not parse compiler option");
        return -1;
    }
    
    temp = string_unpack(&str);
    append_option(options, temp);

    free(temp);
    return result;
}

int parse_string(FILE* file, bld_string* str) {
    int c;

    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c != '\"') {log_warn("Expected string to start with \'\"\', got \'%c\'", c); goto parse_failed;}

    c = getc(file);
    while (c != '\"' && c != EOF) {
        string_append_char(str, c);
        c = getc(file);
    }
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}

    return 0;
    parse_failed:
    return -1;
}

int parse_uintmax(FILE* file, uintmax_t* num_ptr) {
    uintmax_t num = 0;
    int c;

    c = next_character(file);
    if (!isdigit(c)) {
        log_warn("Expected number, got: \'%c\'", c);
        return -1;
    }

    while (c != EOF && isdigit(c)) {
        // Warning: number is assumed to be valid, no overflow etc.
        num = 10 * num + (c - '0');
        c = getc(file);
    }
    ungetc(c, file);

    *num_ptr = num;
    return 0;
}

int next_character(FILE* file) {
    int c = getc(file);
    while (c != EOF && isspace(c)) {c = getc(file);}
    return c;
}

int parse_array(FILE* file, void* obj, bld_parse_func parse_func) {
    int value_num = 0;
    int result, parse_complete;
    int c;

    parse_complete = 0;
    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c != '[') {log_warn("Unexpected starting character: \'%c\'", c); goto parse_failed;}

    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c == ']') {
        parse_complete = 1;
    } else {
        ungetc(c, file);
    }

    while (!parse_complete) {
        value_num += 1;
        result = parse_func(file, obj);
        if (result) {goto parse_failed;}
        
        c = next_character(file);
        switch (c) {
            case (']'): {
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
                goto parse_failed;
            } break;
        }
    }

    return value_num;
    parse_failed:
    return -1;
}

int parse_map(FILE* file, void* obj, int entries, int* parsed, char** keys, bld_parse_func* parse_funcs) {
    int exists, index, key_num = 0;
    int result, parse_complete;
    bld_string str;
    char c, *temp;

    memset(parsed, 0, entries * sizeof(int));

    parse_complete = 0;
    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c != '{') {log_warn("Unexpected starting character: \'%c\'", c); goto parse_failed;}

    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c == '}') {
        parse_complete = 1;
    } else {
        ungetc(c, file);
    }

    while (!parse_complete) {
        key_num += 1;
        str = string_new();
        result = parse_string(file, &str);
        if (result) {
            log_warn("Key %d could not be parsed, expected: [", key_num);
            for (int i = 0; i < entries; i++) {
                if (i > 0) {printf(",\n");}
                printf("  \"%s\"", keys[i]);
            }
            printf("\n]\n");
            goto parse_failed;
        }

        exists = 0;
        temp = string_unpack(&str);
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
            printf("\n]\n");
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

        string_free(&str);
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
                goto parse_failed;
            } break;
        }
    }

    return key_num;
    parse_failed:
    string_free(&str);
    return -1;
}
