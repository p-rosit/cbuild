#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "logging.h"
#include "file.h"
#include "project.h"

bld_project_base    project_base_new(bld_path, bld_compiler, bld_linker);
void                project_base_free(bld_project_base*);

bld_project_cache   project_cache_new(void);
void                project_cache_free(bld_project_cache*);

bld_path            extract_build_path(bld_path*);

bld_forward_project project_new(bld_path path, bld_compiler compiler, bld_linker linker) {
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
    project.resolved = 0;
    project.base = project_base_new(path, compiler, linker);
    project.extra_paths = array_new(sizeof(bld_path));
    project.ignore_paths = set_new(0);
    project.main_file_name.chars = NULL;
    project.compiler_file_names = array_new(sizeof(bld_string));
    project.linker_flags_file_names = array_new(sizeof(bld_string));

    project_ignore_path(&project, path_to_string(&build_file_path));
    path_free(&build_file_path);

    return project;
}

void project_add_build(bld_forward_project* fproject, char* path) {
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

void project_add_path(bld_forward_project* fproject, char* path) {
    bld_path test, extra;

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

void project_ignore_path(bld_forward_project* fproject, char* path) {
    bld_path test;

    if (fproject->resolved) {
        log_fatal("Trying to ignore path \"%s\" but forward project has already been resolved, perform all setup of project before resolving", path);
    }

    test = path_copy(&fproject->base.root);
    path_append_string(&test, path);

    set_add(&fproject->ignore_paths, file_get_id(&test), NULL);
    path_free(&test);
}

void project_set_main_file(bld_forward_project* fproject, char* file_name) {
    bld_string str;

    if (fproject->resolved) {
        log_fatal("Trying to set main file to \"\" but forward project has already been resolved, perform all setup of project before resolving", file_name);
    }

    str = string_new();
    string_append_string(&str, file_name);

    string_free(&fproject->main_file_name);
    fproject->main_file_name = str;
}

void project_set_compiler(bld_forward_project* fproject, char* file_name, bld_compiler compiler) {
    bld_string str;
    bld_compiler_or_flags temp;

    if (fproject->resolved) {
        log_fatal("Trying to set compiler of \"%s\" but forward project has already been resolved, perform all setup of project before resolving", file_name);
    }

    str = string_pack(file_name);
    str = string_copy(&str);

    temp.type = BLD_COMPILER;
    temp.as.compiler = compiler;

    array_push(&fproject->compiler_file_names, &str);
    array_push(&fproject->base.file_compilers, &temp);
}

void project_set_linker_flags(bld_forward_project* fproject, char* file_name, bld_linker_flags flags) {
    bld_string str;

    if (fproject->resolved) {
        log_fatal("Trying to set linker of \"%s\" but forward project has already been resolved, perform all setup of project before resolving", file_name);
    }

    str = string_new();
    string_append_string(&str, file_name);

    array_push(&fproject->linker_flags_file_names, &str);
    array_push(&fproject->base.file_linker_flags, &flags);
}


void project_free(bld_project* project) {
    bld_iter iter;
    bld_file* file;

    project_base_free(&project->base);
    file_tree_free(&project->file_tree);
    dependency_graph_free(&project->graph);

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        file_free(file);
    }
    set_free(&project->files);

    set_free(&project->file2compiler);
    set_free(&project->file2linker_flags);
}

void project_partial_free(bld_forward_project* fproject) {
    bld_iter iter;
    bld_path* path;
    bld_string* str;

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

    iter = iter_array(&fproject->linker_flags_file_names);
    while (iter_next(&iter, (void**) &str)) {
        string_free(str);
    }
    array_free(&fproject->linker_flags_file_names);
}

bld_project_base project_base_new(bld_path path, bld_compiler compiler, bld_linker linker) {
    bld_project_base base;

    base.root = path;
    base.build = path_new();
    base.compiler = compiler;
    base.linker = linker;
    base.file_compilers = array_new(sizeof(bld_compiler_or_flags));
    base.file_linker_flags = array_new(sizeof(bld_linker_flags));
    base.cache = project_cache_new();

    return base;
}

void project_base_free(bld_project_base* base) {
    bld_iter iter;
    bld_compiler_or_flags* compiler;
    bld_linker_flags* linker_flags;

    path_free(&base->root);
    path_free(&base->build);
    compiler_free(&base->compiler);
    linker_free(&base->linker);
    project_cache_free(&base->cache);

    iter = iter_array(&base->file_compilers);
    while (iter_next(&iter, (void**) &compiler)) {
        switch (compiler->type) {
            case (BLD_COMPILER): {
                compiler_free(&compiler->as.compiler);
            } break;
            case (BLD_COMPILER_FLAGS): {
                compiler_flags_free(&compiler->as.flags);
            } break;
            default: log_fatal("project_base_free: internal error");
        }
    }
    array_free(&base->file_compilers);

    iter = iter_array(&base->file_linker_flags);
    while (iter_next(&iter, (void**) &linker_flags)) {
        linker_flags_free(linker_flags);
    }
    array_free(&base->file_linker_flags);
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
    bld_file* file;
    bld_compiler_or_flags* compiler;
    bld_linker_flags* flags;

    if (!cache->loaded) {return;}
    path_free(&cache->root);

    if (!cache->set) {return;}
    compiler_free(&cache->compiler);
    linker_free(&cache->linker);

    iter = iter_set(&cache->files);
    while (iter_next(&iter, (void**) &file)) {
        file_free(file);
    }
    set_free(&cache->files);

    iter = iter_array(&cache->file_compilers);
    while (iter_next(&iter, (void**) &compiler)) {
        switch (compiler->type) {
            case (BLD_COMPILER): {
                compiler_free(&compiler->as.compiler);
            } break;
            case (BLD_COMPILER_FLAGS): {
                compiler_flags_free(&compiler->as.flags);
            } break;
            default: log_fatal("project_cache_free: internal error");
        }
    }
    array_free(&cache->file_compilers);
    set_free(&cache->file2compiler);

    iter = iter_array(&cache->file_linker_flags);
    while (iter_next(&iter, (void**) &flags)) {
        linker_flags_free(flags);
    }
    array_free(&cache->file_linker_flags);
    set_free(&cache->file2linker_flags);

    file_tree_free(&cache->tree);
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
