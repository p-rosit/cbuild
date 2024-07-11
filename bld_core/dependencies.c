#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "logging.h"
#include "graph.h"
#include "dependencies.h"
#include "project.h"
#include "language/language.h"

void parse_included_files(bld_project_base*, bld_file_id, bld_file*, bld_set*);
void parse_symbols(bld_project_base*, bld_file_id, bld_file*);

bld_dependency_graph dependency_graph_new(void) {
    bld_dependency_graph graph;

    graph.include_graph = graph_new();
    graph.symbol_graph = graph_new();

    return graph;
}

void dependency_graph_free(bld_dependency_graph* graph) {
    graph_free(&graph->include_graph);
    graph_free(&graph->symbol_graph);
}

bld_iter dependency_graph_symbols_from(const bld_dependency_graph* graph, bld_file* root) {
    return iter_graph(&graph->symbol_graph, root->identifier.id);
}

bld_iter dependency_graph_includes_from(const bld_dependency_graph* graph, bld_file* root) {
    return  iter_graph(&graph->include_graph, root->identifier.id);
}

int dependency_graph_next_file(bld_iter* iter, const bld_set* files, bld_file** file) {
    uintmax_t file_id;
    int has_next;
    
    has_next = graph_next(&iter->as.graph_iter, &file_id);
    if (!has_next) {return has_next;}

    *file = set_get(files, file_id);
    if (*file == NULL) {log_fatal(LOG_FATAL_PREFIX "file id in graph did not exist in file set.");}

    return has_next;
}

int dependency_graph_extract_includes(bld_dependency_graph* graph, bld_project* project) {
    bld_iter iter;
    bld_file *file;

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        int error;
        bld_compiler compiler;

        if (file->type == BLD_FILE_DIRECTORY) {continue;}

        if (graph_has_node(&graph->include_graph, file->identifier.id)) {
            continue;
        }

        log_debug("Extracting includes of \"%s\"", string_unpack(&file->name));
        graph_add_node(&graph->include_graph, file->identifier.id);

        compiler = file_assemble_compiler(file, &project->files);

        error = cache_includes_get(&project->base.cache_, &compiler, file, &project->files, project->main_file);
        if (error) {
            log_error(LOG_FATAL_PREFIX "could not extract includes of \"%s\"", path_to_string(&file->path));
            return error;
        }

        compiler_assembled_free(&compiler);
    }

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        bld_iter iter;
        bld_file *to_file;

        iter = iter_set(&project->files);
        while (iter_next(&iter, (void**) &to_file)) {
            bld_set* includes;
            if (to_file->type == BLD_FILE_DIRECTORY) {continue;}

            includes = file_includes_get(to_file);

            if (!set_has(includes, file->identifier.id)) {
                continue;
            }

            graph_add_edge(&graph->include_graph, file->identifier.id, to_file->identifier.id);
        }
    }

    log_dinfo("Generated include graph with %lu nodes", graph->include_graph.edges.size);

    return 0;
}

int dependency_graph_extract_symbols(bld_dependency_graph* graph, bld_project* project) {
    bld_iter iter;
    bld_file* file;

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        int error;
        bld_compiler compiler;

        if (file->type == BLD_FILE_DIRECTORY) {continue;}
        if (file->type == BLD_FILE_INTERFACE) {continue;}
        if (!file->compile_successful) {continue;}

        if (graph_has_node(&graph->symbol_graph, file->identifier.id)) {
            continue;
        }

        log_debug("Extracting symbols of \"%s\"", string_unpack(&file->name));
        graph_add_node(&graph->symbol_graph, file->identifier.id);

        compiler = file_assemble_compiler(file, &project->files);

        error = cache_symbols_get(&project->base.cache_, &compiler, file, project->main_file);
        if (error) {
            log_error(LOG_FATAL_PREFIX "could not extract symbols of \"%s\"", path_to_string(&file->path));
            return error;
        }

        compiler_assembled_free(&compiler);
    }

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        bld_iter iter;
        bld_file* to_file;
        bld_set* undefined;

        undefined = file_undefined_get(file);
        if (undefined == NULL) {continue;}

        iter = iter_set(&project->files);
        while (iter_next(&iter, (void**) &to_file)) {
            bld_set* defined;

            defined = file_defined_get(to_file);
            if (defined == NULL) {continue;}

            if (set_empty_intersection(undefined, defined)) {
                continue;
            }
            graph_add_edge(&graph->symbol_graph, file->identifier.id, to_file->identifier.id);
        }
    }

    log_dinfo("Generated symbol graph with %lu nodes", graph->symbol_graph.edges.size);

    return 0;
}

void parse_symbols(bld_project_base* base, bld_file_id main_id, bld_file* file) {
    int error;
    bld_path path;

    if (!base->rebuilding || file->identifier.id != main_id) {
        path = path_copy(&base->root);
    } else {
        path = path_copy(&base->build_of->root);
    }
    path_append_path(&path, &file->path);

    error = language_get_symbols(file->language, base, &path, file);
    if (error) {
        log_fatal("Could not parse symbols of \"%s\"", path_to_string(&path));
    }

    path_free(&path);
}

void parse_included_files(bld_project_base* base, bld_file_id main_id, bld_file* file, bld_set* files) {
    int error;
    bld_path path;

    if (!base->rebuilding || file->identifier.id != main_id) {
        path = path_copy(&base->root);
    } else {
        path = path_copy(&base->build_of->root);
    }
    path_append_path(&path, &file->path);

    error = language_get_includes(file->language, &path, file, files);
    if (error) {
        log_fatal("Could not open file \"%s\"", path_to_string(&path));
    }

    path_free(&path);
}
