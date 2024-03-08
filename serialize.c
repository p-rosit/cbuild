#include <stdio.h>
#include <inttypes.h>
#include "logging.h"
#include "project.h"

void serialize_compiler(FILE*, bld_compiler*, int);
void serialize_compiler_type(FILE*, bld_compiler_type);
void serialize_compiler_options(FILE*, bld_options*, int);

void serialize_files(FILE*, bld_files*, int);
void serialize_file(FILE*, bld_file*, int);
void serialize_file_type(FILE*, bld_file_type);
void serialize_file_id(FILE*, bld_file_identifier);

void serialize_graph(FILE*, bld_graph*, int);
void serialize_node(FILE*, bld_node*, int);
void serialize_functions(FILE*, bld_funcs*, int);
void serialize_includes(FILE*, bld_set*, int);
void serialize_edges(FILE*, bld_edges*, int);

void save_cache(bld_project* project) {
    FILE* cache;
    bld_path cache_path;
    int depth = 1;

    if (project->cache == NULL) {
        log_warn("Trying to save cache but no cache was loaded. Ignoring.");
        return;
    }

    cache_path = copy_path(&project->root);
    append_path(&cache_path, &(*project->cache).root);
    append_dir(&cache_path, BLD_CACHE_NAME);

    cache = fopen(path_to_string(&cache_path), "w");
    if (cache == NULL) {
        log_fatal("Could not open cache file for writing under: \"%s\"", path_to_string(&cache_path));
    }

    fprintf(cache, "{\n");
    fprintf(cache, "  \"compiler\": ");
    serialize_compiler(cache, &project->compiler, depth);
    fprintf(cache, ",\n");

    fprintf(cache, "  \"project_graph\": ");
    serialize_graph(cache, &project->graph, depth);
    fprintf(cache, "\n");

    fprintf(cache, "}\n");

    fclose(cache);
    free_path(&cache_path);
}

void serialize_compiler(FILE* cache, bld_compiler* compiler, int depth) {
    fprintf(cache, "{\n");
    
    fprintf(cache, "%*c\"type\": ", 2 * (depth + 1), ' ');
    serialize_compiler_type(cache, compiler->type);
    fprintf(cache, ",\n");

    fprintf(cache, "%*c\"executable\": \"%s\"", 2 * (depth + 1), ' ', compiler->executable);

    if (compiler->options.array.size > 0) {
        fprintf(cache, ",\n");
        fprintf(cache, "%*c\"options\": ", 2 * (depth + 1), ' ');
        serialize_compiler_options(cache, &compiler->options, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * depth, ' ');
}

void serialize_compiler_type(FILE* cache, bld_compiler_type type) {
    switch (type) {
        case (BLD_GCC): {
            fprintf(cache, "\"gcc\"");
        } break;
        case (BLD_CLANG): {
            fprintf(cache, "\"clang\"");
        } break;
        default: log_fatal("serialize_compiler_type: unreachable error???");
    }
}

void serialize_compiler_options(FILE* cache, bld_options* options, int depth) {
    char** flags = options->array.values;
    fprintf(cache, "[\n");
    
    for (size_t i = 0; i < options->array.size; i++) {
        if (i > 0) {fprintf(cache, ",\n");}
        fprintf(cache, "%*c\"%s\"", 2 * (depth + 1), ' ', flags[i]);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}

void serialize_files(FILE* cache, bld_files* files, int depth) {
    int first = 1;
    bld_iter iter = bld_iter_set(&files->set);
    bld_file* file;

    fprintf(cache, "[\n");
    while (bld_set_next(&iter, (void**) &file)) {
        if (!first) {
            fprintf(cache, ",\n");
        } else {
            first = 0;
        }
        fprintf(cache, "%*c", 2 * (depth + 1), ' ');
        serialize_file(cache, file, depth + 1);
    }
    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}

void serialize_file(FILE* cache, bld_file* file, int depth) {
    fprintf(cache, "{\n");

    fprintf(cache, "%*c\"type\": ", 2 * (depth + 1), ' ');
    serialize_file_type(cache, file->type);
    fprintf(cache, ",\n");

    fprintf(cache, "%*c\"id\": ", 2 * (depth + 1), ' ');
    serialize_file_id(cache, file->identifier);
    fprintf(cache, ",\n");

    fprintf(cache, "%*c\"hash\": %ju,\n", 2 * (depth + 1), ' ', file->identifier.hash);

    fprintf(cache, "%*c\"name\": \"%s\"", 2 * (depth + 1), ' ', make_string(&file->name));

    if (file->compiler != NULL) {
        fprintf(cache, ",\n");
        fprintf(cache, "%*c\"compiler\": ", 2 * (depth + 1), ' ');
        serialize_compiler(cache, file->compiler, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * depth, ' ');
}

void serialize_file_type(FILE* cache, bld_file_type type) {
    switch (type) {
        case (BLD_IMPL): {
            fprintf(cache, "\"implementation\"");
        } break;
        case (BLD_HEADER): {
            fprintf(cache, "\"header\"");
        } break;
        case (BLD_TEST): {
            fprintf(cache, "\"test\"");
        } break;
        default: log_fatal("serialize_file_type: unreachable error???");
    }
}

void serialize_file_id(FILE* cache, bld_file_identifier id) {
    fprintf(cache, "%ju", id.id);
}

void serialize_graph(FILE* cache, bld_graph* graph, int depth) {
    int first = 1;
    bld_iter iter = bld_iter_set(&graph->nodes.set);
    bld_node* node;

    fprintf(cache, "[\n");
    while (bld_set_next(&iter, (void**) &node)) {
        if (!first) {
            fprintf(cache, ",\n");
        } else {
            first = 0;
        }
        fprintf(cache, "%*c", 2 * (depth + 1), ' ');
        serialize_node(cache, node, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}

void serialize_node(FILE* cache, bld_node* node, int depth) {
    fprintf(cache, "{\n");

    fprintf(cache, "%*c\"file\": ", 2 * (depth + 1), ' ');
    serialize_file(cache, node->file, depth + 1);
    fprintf(cache, ",\n");

    fprintf(cache, "%*c\"defined\": ", 2 * (depth + 1), ' ');
    serialize_functions(cache, &node->defined_funcs, depth + 1);
    fprintf(cache, ",\n");

    fprintf(cache, "%*c\"undefined\": ", 2 * (depth + 1), ' ');
    serialize_functions(cache, &node->used_funcs, depth + 1);
    fprintf(cache, ",\n");

    fprintf(cache, "%*c\"edges\": ", 2 * (depth + 1), ' ');
    serialize_edges(cache, &node->edges, depth + 1);
    fprintf(cache, ",\n");

    fprintf(cache, "%*c\"includes\": ", 2 * (depth + 1), ' ');
    serialize_includes(cache, &node->includes, depth + 1);
    fprintf(cache, "\n");

    fprintf(cache, "%*c}", 2 * depth, ' ');
}

void serialize_functions(FILE* cache, bld_funcs* funcs, int depth) {
    int first = 1;
    char** func_ptr = funcs->set.values;
    fprintf(cache, "[\n");

    func_ptr = funcs->set.values;
    for (size_t i = 0; i < funcs->set.capacity + funcs->set.max_offset; i++) {
        if (funcs->set.offset[i] >= funcs->set.max_offset) {continue;}
        if (!first) {fprintf(cache, ",\n");}

        fprintf(cache, "%*c\"%s\"", 2 * (depth + 1), ' ', func_ptr[i]);
        first = 0;
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}

void serialize_includes(FILE* cache, bld_set* includes, int depth) {
    int first = 1;
    if (includes->size == 0) {
        fprintf(cache, "[]");
        return;
    } else if (includes->size == 1) {
        fprintf(cache, "[%ju]", includes->hash[0]);
        return;
    }
    fprintf(cache, "[\n");

    for (size_t i = 0; i < includes->capacity + includes->max_offset; i++) {
        if (includes->offset[i] >= includes->max_offset) {continue;}
        if (!first) {fprintf(cache, ",\n");};
        fprintf(cache, "%*c%ju", 2 * (depth + 1), ' ', includes->hash[i]);
        first = 0;
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}

void serialize_edges(FILE* cache, bld_edges* edges, int depth) {
    size_t i = 0;
    uintmax_t* index;
    bld_iter iter;

    if (edges->array.size == 0) {
        fprintf(cache, "[]");
        return;
    } else if (edges->array.size == 1) {
        index = edges->array.values;
        fprintf(cache, "[%ju]", *index);
        return;
    }
    fprintf(cache, "[\n");

    iter = bld_iter_array(&edges->array);
    while (bld_array_next(&iter, (void**) &index)) {
        if (i > 0) {fprintf(cache, ",\n");};
        fprintf(cache, "%*c%ju", 2 * (depth + 1), ' ', *index);
        i++;
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}
