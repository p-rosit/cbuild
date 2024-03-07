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

int parse_graph(FILE*, bld_project*);
int parse_node(FILE*, bld_project*);
int parse_defined_functions(FILE*, bld_node*);
int parse_undefined_functions(FILE*, bld_node*);
int parse_includes(FILE*, bld_node*);
int parse_edges(FILE*, bld_node*);
int parse_edge(FILE*, bld_edges*);
int parse_function(FILE*, bld_funcs*);
int parse_node_file(FILE*, bld_node*);
int parse_file(FILE*, bld_file*);
int parse_file_type(FILE*, bld_file*);
int parse_file_id(FILE*, bld_file*);
int parse_file_hash(FILE*, bld_file*);
int parse_file_name(FILE*, bld_file*);
int parse_file_compiler(FILE*, bld_file*);

int parse_compiler(FILE*, bld_compiler*);
int parse_compiler_type(FILE*, bld_compiler*);
int parse_compiler_executable(FILE*, bld_compiler*);
int parse_compiler_options(FILE*, bld_compiler*);
int parse_compiler_option(FILE*, bld_options*);

int parse_string(FILE*, bld_string*);
int parse_uintmax(FILE*, uintmax_t*);

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
    int size = 2;
    int parsed[2];
    char *keys[2] = {"compiler", "project_graph"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_project_compiler,
        (bld_parse_func) parse_graph
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

    fclose(f);
    free_path(&path);
    return 0;
}

bld_nodes new_nodes();
int parse_graph(FILE* file, bld_project* project) {
    int amount_parsed;
    bld_file* f;
    bld_node* node;
    bld_files* files = &project->files;
    bld_nodes* nodes = &project->graph.nodes;
    bld_iter iter;

    project->files = new_files();
    project->graph = new_graph(&project->files); 
    amount_parsed = parse_array(file, project, (bld_parse_func) parse_node);
    if (amount_parsed < 0) {
        log_warn("Could not parse graph");
        return -1;
    }

    iter = bld_iter_set(&files->set);
    while (bld_set_next(&iter, (void**) &f)) {
        node = bld_set_get(&nodes->set, f->identifier.id);
        node->file = f;
    }

    return 0;
}

void push_node(bld_nodes*, bld_node);
int parse_node(FILE* file, bld_project* project) {
    int amount_parsed;
    int size = 5;
    int parsed[5];
    char *keys[5] = {"file", "defined", "undefined", "edges", "includes"};
    bld_parse_func funcs[5] = {
        (bld_parse_func) parse_node_file,
        (bld_parse_func) parse_defined_functions,
        (bld_parse_func) parse_undefined_functions,
        (bld_parse_func) parse_edges,
        (bld_parse_func) parse_includes,
    };
    bld_nodes* nodes = &project->graph.nodes;
    bld_files* files = &project->files;
    bld_node node;

    amount_parsed = parse_map(file, &node, size, parsed, keys, funcs);
    append_file(files, *node.file);
    push_node(nodes, node);
    free(node.file);

    if (amount_parsed != size) {
        log_warn("Could not parse node, expected all keys to be present: [");
        for (int i = 0; i < size; i++) {
            if (i > 0) {printf(",\n");}
            printf("  \"%s\"", keys[i]);
        }
        printf("\n]\n");
        return -1;
    }

    return 0;
}

bld_funcs new_funcs();
int parse_functions(FILE* file, bld_funcs* funcs) {
    int values;
    bld_funcs f = new_funcs();

    values = parse_array(file, &f, (bld_parse_func) parse_function);
    if (values < 0) {
        log_warn("Could not parse functions");
        return -1;
    }
    *funcs = f;

    return 0;
}

void add_func(bld_funcs*, bld_string*);
int parse_function(FILE* file, bld_funcs* funcs) {
    bld_string str = new_string();
    int result = parse_string(file, &str);
    if (result) {
        free_string(&str);
        log_warn("Could not parse function");
        return -1;
    }
    
    add_func(funcs, &str);
    return result;
}

int parse_defined_functions(FILE* file, bld_node* node) {
    bld_funcs funcs;
    int result = parse_functions(file, &funcs);
    if (result) {
        log_warn("Could not parse defined functions.");
        return -1;
    }
    node->defined_funcs = funcs;
    return 0;
}

int parse_undefined_functions(FILE* file, bld_node* node) {
    bld_funcs funcs;
    int result = parse_functions(file, &funcs);
    if (result) {
        log_warn("Could not parse defined functions.");
        return -1;
    }
    node->used_funcs = funcs;
    return 0;
}

int parse_include_edge(FILE* file, bld_set* includes) {
    uintmax_t file_id;
    int result = parse_uintmax(file, &file_id);
    if (result) {
        log_warn("Could not parse edge");
        return -1;
    }
    bld_set_add(includes, file_id, NULL);
    return 0;
}

int parse_includes(FILE* file, bld_node* node) {
    int values;
    bld_set includes = bld_set_new(0);

    values = parse_array(file, &includes, (bld_parse_func) parse_include_edge);
    if (values < 0) {
        log_warn("Could not parse ");
        return -1;
    }

    node->includes = includes;
    return 0;
}


bld_edges new_edges();
int parse_edges(FILE* file, bld_node* node) {
    int values;
    bld_edges edges = new_edges();

    values = parse_array(file, &edges, (bld_parse_func) parse_edge);
    if (values < 0) {
        log_warn("Could not parse edges");
        return -1;
    }
    node->edges = edges;
    return 0;
}

void append_edge(bld_edges*, uintmax_t);
int parse_edge(FILE* file, bld_edges* edges) {
    uintmax_t num;
    int result = parse_uintmax(file, &num);
    if (result) {
        log_warn("Could not parse edge");
        return -1;
    }
    append_edge(edges, num);
    return 0;
}

int parse_node_file(FILE* file, bld_node* node) {
    bld_file* f = malloc(sizeof(bld_file));
    int result = parse_file(file, f);
    if (result) {
        free(f);
        log_fatal("Free correctly");
        log_warn("Could not parse file of node.");
        return -1;
    }
    node->file = f;
    return 0;
}

int parse_file(FILE* file, bld_file* f) {
    int amount_parsed;
    int size = 5;
    int parsed[5];
    char *keys[5] = {"type", "id", "hash", "name", "compiler"};
    bld_parse_func funcs[5] = {
        (bld_parse_func) parse_file_type,
        (bld_parse_func) parse_file_id,
        (bld_parse_func) parse_file_hash,
        (bld_parse_func) parse_file_name,
        (bld_parse_func) parse_file_compiler
    };

    f->compiler = NULL;
    amount_parsed = parse_map(file, f, size, parsed, keys, funcs);

    if (amount_parsed < size && parsed[4]) {
        return -1;
    }
    f->path = new_path();
    return 0;
}

int parse_file_type(FILE* file, bld_file* f) {
    bld_string str = new_string();
    char* temp;
    int result = parse_string(file, &str);
    if (result) {
        free_string(&str);
        log_warn("Could not parse compiler type");
        return -1;
    }
    
    temp = make_string(&str);
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

    free_string(&str);
    return result;
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
        /* Warning: number is assumed to be valid, no overflow etc. */
        num = 10 * num + (c - '0');
        c = getc(file);
    }
    ungetc(c, file);

    *num_ptr = num;
    return 0;
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
    bld_string str = new_string();
    int result = parse_string(file, &str);
    if (result) {
        free_string(&str);
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

int parse_project_compiler(FILE* file, bld_project* project) {
    int result = parse_compiler(file, &project->compiler);
    if (result) {log_warn("Could not parse compiler for project.");}
    return result;
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
    bld_string str = new_string();
    char* temp;
    int result = parse_string(file, &str);
    if (result) {
        free_string(&str);
        log_warn("Could not parse compiler type");
        return -1;
    }
    
    temp = make_string(&str);
    if (strcmp(temp, "gcc") == 0) {
        compiler->type = BLD_GCC;
    } else if (strcmp(temp, "clang") == 0) {
        compiler->type = BLD_CLANG;
    } else {
        log_warn("Not a valid compiler type: \"%s\"", temp);
        result = -1;
    }

    free_string(&str);
    return result;
}

int parse_compiler_executable(FILE* file, bld_compiler* compiler) {
    bld_string str = new_string();
    char* temp;
    int result = parse_string(file, &str);
    if (result) {
        free_string(&str);
        log_warn("Could not parse compiler executable");
        return -1;
    }
    
    temp = make_string(&str);
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
    bld_string str = new_string();
    char* temp;
    int result = parse_string(file, &str);
    if (result) {
        free_string(&str);
        log_warn("Could not parse compiler option");
        return -1;
    }
    
    temp = make_string(&str);
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
        append_char(str, c);
        c = getc(file);
    }
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}

    return 0;
    parse_failed:
    return -1;
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
        str = new_string();
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

        free_string(&str);
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
    free_string(&str);
    return -1;
}
