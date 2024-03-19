#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "logging.h"
#include "file.h"
#include "project.h"

int         cached_compilation(bld_project*, bld_file*);

bld_ignore  new_ignore_ids();
void        append_ignore_id(bld_ignore*, uintmax_t);
void        free_ignore_ids(bld_ignore*);


bld_path extract_path(int argc, char** argv) {
    /* TODO: argv[0] is not guaranteed to contain path to executable */
    bld_path path;
    if (argc < 1) {
        log_fatal("Not enough input arguments???");
    }

    path = path_from_string(argv[0]);
    log_debug("Extracted path to executable: \"%.*s\"", (int) path.str.size, path.str.chars);
    return path;
}

bld_path extract_build_path(bld_path* root) {
    bld_path build_path;
    bld_string str = string_new();
    char* name;
    string_append_string(&str, path_get_last_string(root));

    string_unpack(&str);
    if (strncmp(str.chars, "old_", 4) == 0) {
        name = str.chars + 4;
    } else {
        name = str.chars;
    }

    build_path = path_from_string(name);
    path_remove_file_ending(&build_path);
    string_append_string(&build_path.str, ".c");

    string_free(&str);
    return build_path;
}

bld_project make_project(bld_path root, bld_compiler compiler) {
    return (bld_project) {
        .root = root,
        .build = path_new(),
        .extra_paths = array_new(sizeof(bld_path)),
        .ignore_paths = new_ignore_ids(),
        .main_file = 0,
        .compiler = compiler,
        .files = new_files(),
        .changed_files = bld_set_new(sizeof(int)),
        .graph = new_graph(NULL),
        .cache = NULL,
    };
}

bld_project new_project(bld_path root, bld_compiler compiler) {
    FILE* f;
    bld_path build_file_path;
    bld_project project;

    build_file_path = extract_build_path(&root);
    
    path_remove_last_string(&root);
    project = make_project(root, compiler);

    f = fopen(path_to_string(&build_file_path), "r");
    if (f == NULL) {
        log_fatal("Expected executable to have same name (and be in same directory) as build file. Could not find \"%s\"", path_to_string(&build_file_path));
    }
    fclose(f);

    ignore_path(&project, path_to_string(&build_file_path));
    path_free(&build_file_path);

    return project;
}

void free_cache(bld_project* cache) {
    if (cache == NULL) {return;}
    path_free(&cache->root);
    path_free(&cache->build);
    compiler_free(&cache->compiler);
    free_graph(&cache->graph);
    free_files(&cache->files);
    free(cache);
}

void free_project(bld_project* project) {
    bld_iter iter;
    bld_path *path;

    if (project == NULL) {return;}
    path_free(&project->root);
    path_free(&project->build);

    iter = bld_iter_array(&project->extra_paths);
    while (bld_array_next(&iter, (void**) &path)) {
        path_free(path);
    }
    array_free(&project->extra_paths);

    free_ignore_ids(&project->ignore_paths);
    compiler_free(&project->compiler);
    free_graph(&project->graph);
    free_files(&project->files);
    bld_set_free(&project->changed_files);
    free_cache(project->cache);
}

void set_compiler(bld_project* project, char* str, bld_compiler compiler) {
    int match_found = 0;
    bld_path path = path_from_string(str);
    bld_files files = project->files;
    bld_file* file;
    bld_compiler* c_ptr = malloc(sizeof(bld_compiler));
    bld_iter iter;
    if (c_ptr == NULL) {log_fatal("Could not add compiler to \"%s\"", str);}
    *c_ptr = compiler;

    iter = bld_iter_set(&files.set);
    while (bld_set_next(&iter, (void**) &file)) {
        if (path_ends_with(&file->path, &path)) {
            if (match_found) {
                log_fatal("Name of file \"%s\" is ambiguous, found several matches.", str);
            }
            match_found = 1;
            file->compiler = c_ptr;
        }
    }

    path_free(&path);
    if (!match_found) {
        log_fatal("No file matching \"%s\" could be found.", str);
    }
}

void set_main_file(bld_project* project, char* str) {
    int match_found = 0;
    bld_path path = path_from_string(str);
    bld_files files = project->files;
    bld_file* file;
    bld_iter iter = bld_iter_set(&files.set);

    while (bld_set_next(&iter, (void**) &file)) {
        if (path_ends_with(&file->path, &path)) {
            if (match_found) {
                log_fatal("Name of main file \"%s\" is ambiguous, found several matches.", str);
            }
            match_found = 1;
            project->main_file = file->identifier.id;
        }
    }

    path_free(&path);
    if (!match_found) {
        log_fatal("No file matching \"%s\" could be found.", str);
    }
}

void add_build(bld_project* project, char* path) {
    path_free(&project->build);
    project->build = path_from_string(path);
    ignore_path(project, path);
}

void add_path(bld_project* project, char* path) {
    bld_path test, extra;
    
    test = path_copy(&project->root);
    path_append_string(&test, path);
    get_file_id(&test);

    extra = path_from_string(path);
    bld_array_push(&project->extra_paths, &extra);

    path_free(&test);
}

void ignore_path(bld_project* project, char* path) {
    bld_path test = path_copy(&project->root);
    path_append_string(&test, path);

    append_ignore_id(&project->ignore_paths, get_file_id(&test));

    path_free(&test);
}

bld_ignore new_ignore_ids() {
    return (bld_ignore) {.set = bld_set_new(0)};
}

void free_ignore_ids(bld_ignore* ignore) {
    bld_set_free(&ignore->set);
}

void append_ignore_id(bld_ignore* ignore, uintmax_t id) {
    bld_set_add(&ignore->set, id, NULL);
}
