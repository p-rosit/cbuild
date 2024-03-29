#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "logging.h"
#include "file.h"
#include "project.h"

bld_project_base    project_base_new(bld_path, bld_compiler);
void                project_base_free(bld_project_base*);

bld_project_cache   project_cache_new(void);
void                project_cache_free(bld_project_cache*);

bld_path            extract_build_path(bld_path*);

bld_forward_project project_new(bld_path path, bld_compiler compiler) {
    FILE* f;
    bld_path build_file_path;
    bld_forward_project project;
    
    build_file_path = extract_build_path(&path);
    path_remove_last_string(&path);

    f = fopen(path_to_string(&build_file_path), "r");
    if (f == NULL) {
        log_fatal("Expected executable to have same name (and be in same directory) as build file. Could not find \"%s\"", path_to_string(&build_file_path));
    } else {
        fclose(f);
    }

    project.rebuilding = 0;
    project.base = project_base_new(path, compiler);
    project.extra_paths = array_new(sizeof(bld_path));
    project.ignore_paths = set_new(0);
    project.main_file_name.chars = NULL;
    project.file_names = array_new(sizeof(bld_string));

    project_ignore_path(&project, path_to_string(&build_file_path));
    path_free(&build_file_path);

    return project;
}

void project_add_build(bld_forward_project* project, char* path) {
    char* current_build = path_to_string(&project->base.build);

    if (current_build[0] != '\0') {
        log_fatal("Trying to add build path to project but build path has already been set to \"%s\"", current_build);
    }

    path_free(&project->base.build);
    project->base.build = path_from_string(path);
    project_ignore_path(project, path);
}

void project_add_path(bld_forward_project* project, char* path) {
    bld_path test, extra;

    test = path_copy(&project->base.root);
    path_append_string(&test, path);
    file_get_id(&test);
    path_free(&test);

    extra = path_from_string(path);
    array_push(&project->extra_paths, &extra);
}

void project_ignore_path(bld_forward_project* project, char* path) {
    bld_path test = path_copy(&project->base.root);
    path_append_string(&test, path);

    set_add(&project->ignore_paths, file_get_id(&test), NULL);
    path_free(&test);
}

void project_set_main_file(bld_forward_project* project, char* file_name) {
    bld_string str = string_new();
    string_append_string(&str, file_name);

    string_free(&project->main_file_name);
    project->main_file_name = str;
}

void project_set_compiler(bld_forward_project* project, char* file_name, bld_compiler compiler) {
    bld_string str = string_new();
    string_append_string(&str, file_name);

    array_push(&project->file_names, &str);
    array_push(&project->base.file_compilers, &compiler);
}


void project_free(bld_project* project) {
    bld_iter iter;
    bld_file* file;

    project_base_free(&project->base);
    dependency_graph_free(&project->graph);

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        file_free(file);
    }
    set_free(&project->files);
}

bld_project_base project_base_new(bld_path path, bld_compiler compiler) {
    bld_project_base base;

    base.root = path;
    base.build = path_new();
    base.compiler = compiler;
    base.file_compilers = array_new(sizeof(bld_compiler));
    base.cache = project_cache_new();

    return base;
}

void project_base_free(bld_project_base* base) {
    bld_iter iter;
    bld_compiler* compiler;

    path_free(&base->root);
    path_free(&base->build);
    compiler_free(&base->compiler);
    project_cache_free(&base->cache);

    iter = iter_array(&base->file_compilers);
    while (iter_next(&iter, (void**) &compiler)) {
        compiler_free(compiler);
    }
    array_free(&base->file_compilers);
}

bld_project_cache project_cache_new(void) {
    bld_project_cache cache;

    cache.loaded = 0;
    cache.set = 0;
    cache.applied = 0;

    return cache;
}

void project_cache_free(bld_project_cache* cache) {
    bld_iter iter;
    bld_file *file;
    
    if (!cache->loaded) {return;}
    path_free(&cache->root);

    if (!cache->set) {return;}
    compiler_free(&cache->compiler);

    iter = iter_set(&cache->files);
    while (iter_next(&iter, (void**) &file)) {
        file_free(file);
    }
    set_free(&cache->files);
}

bld_path project_path_extract(int argc, char** argv) {
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
