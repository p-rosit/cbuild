#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "logging.h"
#include "file.h"
#include "project.h"

bld_forward_project project_forward_new(bld_path*, bld_compiler*, bld_linker*);

bld_project_base    project_base_new(bld_path*, bld_linker*);
void                project_base_free(bld_project_base*);

bld_project_cache   project_cache_new(void);
void                project_cache_free(bld_project_cache*);

bld_string          extract_build_name(bld_path*);

bld_forward_project project_new(bld_path path, bld_compiler compiler, bld_linker linker) {
    FILE* f;
    bld_path build_file_path;
    bld_string build_file_name;
    bld_forward_project fproject;
    
    build_file_name = extract_build_name(&path);
    path_remove_last_string(&path);
    build_file_path = path_copy(&path);
    path_append_string(&build_file_path, string_unpack(&build_file_name));

    f = fopen(path_to_string(&build_file_path), "r");
    if (f == NULL) {
        log_fatal("Expected executable to have same name (and be in same directory) as build file. Could not find \"%s\"", path_to_string(&build_file_path));
    } else {
        fclose(f);
    }

    fproject = project_forward_new(&path, &compiler, &linker);

    project_ignore_path(&fproject, string_unpack(&build_file_name));
    path_free(&build_file_path);
    string_free(&build_file_name);

    return fproject;
}

bld_forward_project project_forward_new(bld_path* path, bld_compiler* compiler, bld_linker* linker) {
    bld_forward_project fproject;
    fproject.resolved = 0;
    fproject.base = project_base_new(path, linker);
    fproject.extra_paths = set_new(0);
    fproject.ignore_paths = set_new(0);
    fproject.main_file_name.chars = NULL;
    fproject.compiler = *compiler;
    fproject.compiler_file_names = array_new(sizeof(bld_string));
    fproject.file_compilers = array_new(sizeof(bld_compiler_or_flags));
    fproject.linker_flags_file_names = array_new(sizeof(bld_string));
    fproject.file_linker_flags = array_new(sizeof(bld_linker_flags));

    set_add(&fproject.base.compiler_handles, compiler->type, &compiler->type);
    return fproject;
}

void project_add_build(bld_forward_project* fproject, char* path) {
    if (fproject->resolved) {
        log_fatal("Trying to add build path \"\" but forward project has already been resolved, perform all setup of project before resolving", path);
    }

    if (!fproject->base.standalone) {
        log_fatal("Trying to add build path to project but build path has already been set to \"%s\"", path_to_string(&fproject->base.build));
    }

    fproject->base.standalone = 0;
    fproject->base.build = path_from_string(path);
}

void project_add_path(bld_forward_project* fproject, char* path) {
    bld_path test;

    if (fproject->resolved) {
        log_fatal("Trying to add path \"\" but forward project has already been resolved, perform all setup of project before resolving", path);
    }

    test = path_copy(&fproject->base.root);
    path_append_string(&test, path);

    set_add(&fproject->extra_paths, file_get_id(&test), NULL);
    path_free(&test);
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
    array_push(&fproject->file_compilers, &temp);

    if (!set_has(&fproject->base.compiler_handles, compiler.type)) {
        set_add(&fproject->base.compiler_handles, compiler.type, &compiler.type);
    }
}

void project_set_compiler_flags(bld_forward_project* fproject, char* file_name, bld_compiler_flags flags) {
    bld_string str;
    bld_compiler_or_flags temp;

    if (fproject->resolved) {
        log_fatal("Trying to set compiler flags of \"%s\" but forward project has already been resolved, perform all setup of project before resolving", file_name);
    }

    str = string_pack(file_name);
    str = string_copy(&str);

    temp.type = BLD_COMPILER_FLAGS;
    temp.as.flags = flags;

    array_push(&fproject->compiler_file_names, &str);
    array_push(&fproject->file_compilers, &temp);
}

void project_set_linker_flags(bld_forward_project* fproject, char* file_name, bld_linker_flags flags) {
    bld_string str;

    if (fproject->resolved) {
        log_fatal("Trying to set linker of \"%s\" but forward project has already been resolved, perform all setup of project before resolving", file_name);
    }

    str = string_new();
    string_append_string(&str, file_name);

    array_push(&fproject->linker_flags_file_names, &str);
    array_push(&fproject->file_linker_flags, &flags);
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

void project_partial_free(bld_forward_project* fproject) {
    bld_iter iter;
    bld_string* str;

    set_free(&fproject->extra_paths);
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

bld_project_base project_base_new(bld_path* path, bld_linker* linker) {
    bld_project_base base;

    base.rebuilding = 0;
    base.build_of = NULL;
    base.root = *path;
    base.standalone = 1;
    base.compiler_handles = set_new(sizeof(bld_compiler_type));
    base.linker = *linker;
    base.cache = project_cache_new();

    return base;
}

void project_base_free(bld_project_base* base) {
    path_free(&base->root);
    if (!base->standalone) {
        path_free(&base->build);
    }
    set_free(&base->compiler_handles);
    linker_free(&base->linker);
    project_cache_free(&base->cache);
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

bld_string extract_build_name(bld_path* root) {
    bld_path path;
    bld_string build_name;
    char* name;

    path = path_copy(root);
    path_remove_file_ending(&path);
    name = path_get_last_string(&path);
    name += (4 * strncmp(name, "old_", 4) == 0);

    build_name = string_pack(name);
    build_name = string_copy(&build_name);
    string_append_string(&build_name, ".c");

    path_free(&path);
    return build_name;
}
