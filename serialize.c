#include <inttypes.h>
#include "build.h"
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
    append_path(&cache_path, &(*project->cache).path);
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

    if (compiler->options.size > 0) {
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
    fprintf(cache, "[\n");
    
    for (size_t i = 0; i < options->size; i++) {
        if (i > 0) {fprintf(cache, ",\n");}
        fprintf(cache, "%*c\"%s\"", 2 * (depth + 1), ' ', options->options[i]);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}

void serialize_files(FILE* cache, bld_files* files, int depth) {
    fprintf(cache, "[\n");
    for (size_t i = 0; i < files->size; i++) {
        if (i > 0) {
            fprintf(cache, ",\n");
        }
        fprintf(cache, "%*c", 2 * (depth + 1), ' ');
        serialize_file(cache, &files->files[i], depth + 1);
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

    fprintf(cache, "%*c\"hash\": 0,\n", 2 * (depth + 1), ' ');

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
    fprintf(cache, "[\n");

    for (size_t i = 0; i < graph->nodes.size; i++) {
        if (i > 0) {fprintf(cache, ",\n");}
        fprintf(cache, "%*c", 2 * (depth + 1), ' ');
        serialize_node(cache, &graph->nodes.nodes[i], depth + 1);
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
    fprintf(cache, "\n");

    fprintf(cache, "%*c}", 2 * depth, ' ');
}

void serialize_functions(FILE* cache, bld_funcs* funcs, int depth) {
    fprintf(cache, "[\n");

    for (size_t i = 0; i < funcs->size; i++) {
        if (i > 0) {fprintf(cache, ",\n");}
        fprintf(cache, "%*c\"%s\"", 2 * (depth + 1), ' ', funcs->funcs[i]);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}

void serialize_edges(FILE* cache, bld_edges* edges, int depth) {
    if (edges->size == 0) {
        fprintf(cache, "[]");
        return;
    } else if (edges->size == 1) {
        fprintf(cache, "[%ju]", (uintmax_t) 0);
        return;
    }
    fprintf(cache, "[\n");

    for (size_t i = 0; i < edges->size; i++) {
        if (i > 0) {fprintf(cache, ",\n");};
        fprintf(cache, "%*c%p", 2 * (depth + 1), ' ', edges->nodes[i]);
        // fprintf(cache, "%*c%ju", 2 * (depth + 1), ' ', (uintmax_t) edges->nodes[i]->file->identifier.id);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}
