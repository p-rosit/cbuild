#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "logging.h"
#include "file.h"
#include "project.h"

bit_project_base    project_base_new(bit_path, bit_linker);
void                project_base_free(bit_project_base*);

bit_project_cache   project_cache_new(void);
void                project_cache_free(bit_project_cache*);

bit_path            extract_build_path(bit_path*);

bit_forward_project project_new(bit_path path, bit_compiler compiler, bit_linker linker) {
    FILE* f;
    bit_path build_file_path;
    bit_forward_project project;
    
    build_file_path = extract_build_path(&path);
    path_remove_last_string(&path);

    f = fopen(path_to_string(&build_file_path), "r");
    if (f == NULL) {
        log_fatal("Expected executable to have same name (and be in same directory) as build file. Could not find \"%s\"", path_to_string(&build_file_path));
    } else {
        fclose(f);
    }

    project.rebuilding = 0;
    project.resolved = 0;
    project.base = project_base_new(path, linker);
    project.extra_paths = array_new(sizeof(bit_path));
    project.ignore_paths = set_new(0);
    project.main_file_name.chars = NULL;
    project.compiler = compiler;
    project.compiler_file_names = array_new(sizeof(bit_string));
    project.file_compilers = array_new(sizeof(bit_compiler_or_flags));
    project.linker_flags_file_names = array_new(sizeof(bit_string));
    project.file_linker_flags = array_new(sizeof(bit_linker_flags));

    project_ignore_path(&project, path_to_string(&build_file_path));
    path_free(&build_file_path);

    return project;
}

void project_add_build(bit_forward_project* fproject, char* path) {
    char* current_build = path_to_string(&fproject->base.build);

    if (fproject->resolved) {
        log_fatal("Trying to add build path \"\" but forward project has already been resolved, perform all setup of project before resolving", path);
    }

    if (current_build[0] != '\0') {
        log_fatal("Trying to add build path to project but build path has already been set to \"%s\"", current_build);
    }

    path_free(&fproject->base.build);
    fproject->base.build = path_from_string(path);
    project_ignore_path(fproject, path);
}

void project_add_path(bit_forward_project* fproject, char* path) {
    bit_path test, extra;

    if (fproject->resolved) {
        log_fatal("Trying to add path \"\" but forward project has already been resolved, perform all setup of project before resolving", path);
    }

    test = path_copy(&fproject->base.root);
    path_append_string(&test, path);
    file_get_id(&test);
    path_free(&test);

    extra = path_from_string(path);
    array_push(&fproject->extra_paths, &extra);
}

void project_ignore_path(bit_forward_project* fproject, char* path) {
    bit_path test;

    if (fproject->resolved) {
        log_fatal("Trying to ignore path \"%s\" but forward project has already been resolved, perform all setup of project before resolving", path);
    }

    test = path_copy(&fproject->base.root);
    path_append_string(&test, path);

    set_add(&fproject->ignore_paths, file_get_id(&test), NULL);
    path_free(&test);
}

void project_set_main_file(bit_forward_project* fproject, char* file_name) {
    bit_string str;

    if (fproject->resolved) {
        log_fatal("Trying to set main file to \"\" but forward project has already been resolved, perform all setup of project before resolving", file_name);
    }

    str = string_new();
    string_append_string(&str, file_name);

    string_free(&fproject->main_file_name);
    fproject->main_file_name = str;
}

void project_set_compiler(bit_forward_project* fproject, char* file_name, bit_compiler compiler) {
    bit_string str;
    bit_compiler_or_flags temp;

    if (fproject->resolved) {
        log_fatal("Trying to set compiler of \"%s\" but forward project has already been resolved, perform all setup of project before resolving", file_name);
    }

    str = string_pack(file_name);
    str = string_copy(&str);

    temp.type = BIT_COMPILER;
    temp.as.compiler = compiler;

    array_push(&fproject->compiler_file_names, &str);
    array_push(&fproject->file_compilers, &temp);
}

void project_set_linker_flags(bit_forward_project* fproject, char* file_name, bit_linker_flags flags) {
    bit_string str;

    if (fproject->resolved) {
        log_fatal("Trying to set linker of \"%s\" but forward project has already been resolved, perform all setup of project before resolving", file_name);
    }

    str = string_new();
    string_append_string(&str, file_name);

    array_push(&fproject->linker_flags_file_names, &str);
    array_push(&fproject->file_linker_flags, &flags);
}


void project_free(bit_project* project) {
    bit_iter iter;
    bit_file* file;

    project_base_free(&project->base);
    dependency_graph_free(&project->graph);

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        file_free(file);
    }
    set_free(&project->files);
}

void project_partial_free(bit_forward_project* fproject) {
    bit_iter iter;
    bit_path* path;
    bit_string* str;

    iter = iter_array(&fproject->extra_paths);
    while (iter_next(&iter, (void**) &path)) {
        path_free(path);
    }
    array_free(&fproject->extra_paths);

    set_free(&fproject->ignore_paths);
    string_free(&fproject->main_file_name);

    iter = iter_array(&fproject->compiler_file_names);
    while (iter_next(&iter, (void**) &str)) {
        string_free(str);
    }
    array_free(&fproject->compiler_file_names);
    array_free(&fproject->file_compilers);

    iter = iter_array(&fproject->linker_flags_file_names);
    while (iter_next(&iter, (void**) &str)) {
        string_free(str);
    }
    array_free(&fproject->linker_flags_file_names);
    array_free(&fproject->file_linker_flags);
}

bit_project_base project_base_new(bit_path path, bit_linker linker) {
    bit_project_base base;

    base.root = path;
    base.build = path_new();
    base.linker = linker;
    base.cache = project_cache_new();

    return base;
}

void project_base_free(bit_project_base* base) {
    path_free(&base->root);
    path_free(&base->build);
    linker_free(&base->linker);
    project_cache_free(&base->cache);
}

bit_project_cache project_cache_new(void) {
    bit_project_cache cache;

    cache.loaded = 0;
    cache.set = 0;
    cache.applied = 0;

    return cache;
}

void project_cache_free(bit_project_cache* cache) {
    bit_iter iter;
    bit_file* file;

    if (!cache->loaded) {return;}
    path_free(&cache->root);

    if (!cache->set) {return;}
    linker_free(&cache->linker);

    iter = iter_set(&cache->files);
    while (iter_next(&iter, (void**) &file)) {
        file_free(file);
    }
    set_free(&cache->files);
}

bit_path project_path_extract(int argc, char** argv) {
    /* TODO: argv[0] is not guaranteed to contain path to executable */
    bit_path path;
    if (argc < 1) {
        log_fatal("Not enough input arguments???");
    }

    path = path_from_string(argv[0]);
    log_debug("Extracted path to executable: \"%.*s\"", (int) path.str.size, path.str.chars);
    return path;
}

bit_path extract_build_path(bit_path* root) {
    bit_path build_path;
    bit_string str = string_new();
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
