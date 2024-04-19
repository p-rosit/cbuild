#include <stdio.h>
#include <string.h>
#include "os.h"
#include "logging.h"
#include "incremental.h"

void    incremental_make_root(bld_project*);
void    incremental_index_project(bld_project*, bld_forward_project*);
void    incremental_index_possible_file(bld_project*, uintmax_t, bld_path*, char*);
void    incremental_index_recursive(bld_project*, bld_forward_project*, uintmax_t, bld_path*, char*);
void    incremental_apply_main_file(bld_project*, bld_forward_project*);
void    incremental_apply_compilers(bld_project*, bld_forward_project*);
void    incremental_apply_linker_flags(bld_project*, bld_forward_project*);

int     incremental_compile_file(bld_project*, bld_file*);
int     incremental_link_executable(bld_project*, char*);
int     incremental_compile_with_absolute_path(bld_project*, char*);

void    incremental_mark_changed_files(bld_project*, bld_set*);
int     incremental_cached_compilation(bld_project*, bld_file*);
int     incremental_compile_changed_files(bld_project*, bld_set*, int*);

bld_project project_resolve(bld_forward_project* fproject) {
    bld_project project;
    uintmax_t hash;
    bld_iter iter;
    bld_file* file;

    project.base = fproject->base;
    project.files = set_new(sizeof(bld_file));
    project.file_tree = file_tree_new();
    project.graph = dependency_graph_new();

    incremental_make_root(&project);

    if (fproject->rebuilding) {
        char* main_name;
        bld_path main_path;
        main_path.str = fproject->main_file_name;

        main_name = path_get_last_string(&main_path);
        incremental_index_possible_file(&project, project.root_dir, &main_path, main_name);
    }
    incremental_index_project(&project, fproject);

    incremental_apply_main_file(&project, fproject);
    incremental_apply_compilers(&project, fproject);
    incremental_apply_linker_flags(&project, fproject);

    hash = compiler_hash(&project.base.compiler);
    iter = iter_set(&project.files);
    while (iter_next(&iter, (void**) &file)) {
        file->identifier.hash = file_hash(file, &project.base.file_compilers, hash);
    }

    if (project.base.cache.set) {
        incremental_apply_cache(&project);
    }

    fproject->resolved = 1;
    project_partial_free(fproject);

    return project;
}

void incremental_apply_cache(bld_project* project) {
    bld_iter iter;
    bld_file *file, *cached;

    if (!project->base.cache.loaded) {
        log_fatal("Trying to apply cache but no cache has been loaded");
    }

    log_debug("Applying cache under \"%s\"", path_to_string(&project->base.cache.root));

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        if (file->type == BLD_DIR) {continue;}

        cached = set_get(&project->base.cache.files, file->identifier.id);
        if (cached == NULL) {continue;}

        if (file->identifier.hash != cached->identifier.hash) {continue;}

        switch (file->type) {
            case (BLD_IMPL): {
                log_debug("Found \"%s\" in cache: %lu include(s), %lu undefined, %lu defined", string_unpack(&file->name), cached->info.impl.includes.size, cached->info.impl.undefined_symbols.size, cached->info.impl.defined_symbols.size);
                file->info.impl.includes = set_copy(&cached->info.impl.includes);
            } break;
            case (BLD_HEADER): {
                log_debug("Found \"%s\" in cache: %lu include(s)", string_unpack(&file->name), cached->info.header.includes.size);
                file->info.header.includes = set_copy(&cached->info.header.includes);
            } break;
            case (BLD_TEST): {
                log_debug("Found \"%s\" in cache: %lu include(s), %lu undefined", string_unpack(&file->name), cached->info.test.includes.size, cached->info.test.undefined_symbols.size);
                file->info.test.includes = set_copy(&cached->info.test.includes);
            } break;
            default: {log_fatal("incremental_apply_cache: unrecognized file type, unreachable error");}
        }

        graph_add_node(&project->graph.include_graph, file->identifier.id);

        if (file->type == BLD_HEADER) {continue;}

        file_symbols_copy(file, cached);
        graph_add_node(&project->graph.symbol_graph, file->identifier.id);
    }

    project->base.cache.applied = 1;
}


void incremental_index_possible_file(bld_project* project, uintmax_t parent_dir, bld_path* path, char* name) {
    int exists;
    char* file_ending;
    bld_path file_path;
    bld_file file;
    (void)(parent_dir);

    file_ending = strrchr(name, '.');
    if (file_ending == NULL) {return;}

    file_path = path_copy(path);
    if (strncmp(name, "test", 4) == 0 && strcmp(file_ending, ".c") == 0) {
        file = file_test_new(&file_path, name);
    } else if (strcmp(file_ending, ".c") == 0) {
        file = file_impl_new(&file_path, name);
    } else if (strcmp(file_ending, ".h") == 0) {
        file = file_header_new(&file_path, name);
    } else {
        path_free(&file_path);
        return;
    }

    exists = set_add(&project->files, file.identifier.id, &file);
    if (exists) {
        log_error("encountered \"%s\" multiple times while indexing", string_unpack(&file.name));
        file_free(&file);
    }

    file_tree_add(&project->file_tree, parent_dir, file.identifier.id);
}

void incremental_index_recursive(bld_project* project, bld_forward_project* forward_project, uintmax_t parent_dir, bld_path* path, char* name) {
    uintmax_t current_id;
    char *str_path, *file_name;
    bld_path dir_path, sub_path;
    bld_os_dir* dir;
    bld_os_file* file_ptr;
    (void)(parent_dir);

    str_path = path_to_string(path);
    dir = os_dir_open(str_path);
    if (dir == NULL) {
        incremental_index_possible_file(project, parent_dir, path, name);
        return;
    }

    if (name == NULL) {
        log_debug("Searching under root: \"%s\"", path_to_string(path));
    } else {
        log_debug("Searching under: \"%s\": named \"%s\"", path_to_string(path), name);
    }

    if (name != NULL) {
        int exists;
        bld_file dir;

        dir_path = path_copy(path);
        dir = file_dir_new(&dir_path, name);
        exists = set_add(&project->files, dir.identifier.id, &dir);
        if (exists) {
            log_error("encountered \"%s\" multiple times while indexing", string_unpack(&dir.name));
            file_free(&dir);
            return;
        }

        current_id = dir.identifier.id;
        file_tree_add(&project->file_tree, parent_dir, current_id);
    } else {
        current_id = project->root_dir;
    }

    while ((file_ptr = os_dir_read(dir)) != NULL) {
        file_name = os_file_name(file_ptr);
        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
            continue;
        }
        if (file_name[0] == '.') {
            continue;
        }
        if (set_has(&forward_project->ignore_paths, os_file_id(file_ptr))) {
            log_debug("Ignoring: \"%s" BLD_PATH_SEP "%s\"", path_to_string(path), file_name);
            continue;
        }

        sub_path = path_copy(path);
        path_append_string(&sub_path, file_name);
        incremental_index_recursive(project, forward_project, current_id, &sub_path, file_name);
        path_free(&sub_path);
    }
    
    os_dir_close(dir);
}

void incremental_index_project(bld_project* project, bld_forward_project* forward_project) {
    bld_os_dir* dir;
    char* name;
    bld_path *path, extra_path;
    bld_iter iter;

    dir = os_dir_open(path_to_string(&forward_project->base.root));
    if (dir == NULL) {log_fatal("Could not open project root \"%s\"", path_to_string(&forward_project->base.root));}
    os_dir_close(dir);

    log_info("Indexing project under root");
    incremental_index_recursive(project, forward_project, project->root_dir, &forward_project->base.root, NULL);

    iter = iter_array(&forward_project->extra_paths);
    while (iter_next(&iter, (void**) &path)) {
        extra_path = path_copy(&project->base.root);
        path_append_path(&extra_path, path);
        name = path_get_last_string(&extra_path);
        log_info("Indexing files under \"%s\"", path_to_string(&extra_path));

        incremental_index_recursive(project, forward_project, project->root_dir, &extra_path, name);
        path_free(&extra_path);
    }
}

void incremental_make_root(bld_project* project) {
    int exists;
    bld_path path = path_copy(&project->base.root);
    bld_file root = file_dir_new(&path, ".");

    project->root_dir = root.identifier.id;
    exists = set_add(&project->files, root.identifier.id, &root);

    if (exists) {log_fatal("incremental_make_root: root has already been initialized, has the project already been resolved?");}

    file_tree_set_root(&project->file_tree, root.identifier.id);
}

void incremental_apply_main_file(bld_project* project, bld_forward_project* fproject) {
    int match_found = 0;
    bld_iter iter;
    bld_path path;
    bld_file* file;

    path.str = fproject->main_file_name;

    if (fproject->rebuilding) {
        project->main_file = file_get_id(&path);
        return;
    }

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        if (path_ends_with(&file->path, &path)) {
            if (match_found) {
                log_fatal("Name of main file \"%s\" is ambiguous, found several matches", string_unpack(&fproject->main_file_name));
            }
            match_found = 1;
            project->main_file = file->identifier.id;
        }
    }

    if (!match_found) {
        log_fatal("No file matching \"%s\" could be found", string_unpack(&fproject->main_file_name));
    }
}

void incremental_apply_compilers(bld_project* project, bld_forward_project* fproject) {
    size_t index;
    bld_iter iter;
    bld_string* file_name;

    if (fproject->compiler_file_names.size != fproject->base.file_compilers.size) {
        log_fatal("incremental_apply_compilers: internal error, there is not an equal amount of files and compilers");
    }

    index = 0;
    iter = iter_array(&fproject->compiler_file_names);
    while (iter_next(&iter, (void**) &file_name)) {
        int match_found;
        bld_iter iter;
        bld_file* file;
        bld_path path;

        match_found = 0;
        path = path_from_string(string_unpack(file_name));
        iter = iter_set(&project->files);
        while (iter_next(&iter, (void**) &file)) {
            if (path_ends_with(&file->path, &path)) {
                if (match_found) {
                    log_fatal("Applying compiler to \"%s\" but several matches were found, specify more of path to determine exact match", string_unpack(file_name));
                }
                match_found = 1;
                file->compiler = index;
            }
        }

        path_free(&path);

        index++;
    }
}

void incremental_apply_linker_flags(bld_project* project, bld_forward_project* fproject) {
    size_t index;
    bld_iter iter;
    bld_string* file_name;

    index = 0;
    iter = iter_array(&fproject->linker_flags_file_names);
    while (iter_next(&iter, (void**) &file_name)) {
        int match_found;
        bld_iter iter;
        bld_file* file;
        bld_path path;

        match_found = 0;
        path = path_from_string(string_unpack(file_name));
        iter = iter_set(&project->files);
        while (iter_next(&iter, (void**) &file)) {
            if (path_ends_with(&file->path, &path)) {
                if (match_found) {
                    log_fatal("Applying compiler to \"%s\" but several matches were found, specify more of path to determine exact match", string_unpack(file_name));
                }
                match_found = 1;
                file->linker_flags = index;
            }
        }

        path_free(&path);

        index++;
    }
}

int incremental_compile_file(bld_project* project, bld_file* file) {
    int result;
    char name[FILENAME_MAX];
    bld_iter iter;
    bld_compiler* compiler;
    bld_string cmd = string_new();
    bld_string* flag;
    bld_path path;

    if (file->compiler > 0) {
        compiler = array_get(&project->base.file_compilers, file->compiler);
    } else {
        compiler = &project->base.compiler;
    }

    string_append_string(&cmd, string_unpack(&compiler->executable));
    string_append_space(&cmd);

    string_append_string(&cmd, "-c ");
    string_append_string(&cmd, path_to_string(&file->path));

    string_append_string(&cmd, " -o ");

    path = path_copy(&project->base.root);
    if (project->base.cache.loaded) {
        path_append_path(&path, &project->base.cache.root);
    }
    serialize_identifier(name, file);
    path_append_string(&path, name);
    string_append_string(&cmd, path_to_string(&path));

    string_append_string(&cmd, ".o");
    path_free(&path);

    iter = iter_array(&compiler->flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_append_space(&cmd);
        string_append_string(&cmd, string_unpack(flag));
    }

    result = system(string_unpack(&cmd));
    string_free(&cmd);
    return result;
}

int incremental_link_executable(bld_project* project, char* executable_name) {
    int result;
    char name[FILENAME_MAX];
    bld_path path;
    bld_file *main_file, *file;
    bld_string cmd;
    bld_array linker_flags;
    bld_linker_flags* flags;
    bld_iter iter;

    main_file = set_get(&project->files, project->main_file);
    if (main_file == NULL) {
        log_fatal("No main file has been set");
        return 1;
    }

    cmd = string_new();
    string_append_string(&cmd, string_unpack(&project->base.linker.executable));
    string_append_space(&cmd);

    string_append_string(&cmd, "-o ");
    string_append_string(&cmd, executable_name);

    linker_flags = array_new(sizeof(bld_linker_flags*));
    iter = dependency_graph_symbols_from(&project->graph, main_file);
    while (dependency_graph_next_file(&iter, &project->files, &file)) {
        string_append_space(&cmd);

        path = path_copy(&project->base.root);
        if (project->base.cache.loaded) {
            path_append_path(&path, &project->base.cache.root);
        }
        serialize_identifier(name, file);
        path_append_string(&path, name);
        
        string_append_string(&cmd, path_to_string(&path));
        string_append_string(&cmd, ".o");
        path_free(&path);

        if (file->linker_flags < 0) {continue;}
        flags = array_get(&project->base.file_linker_flags, file->linker_flags);
        array_push(&linker_flags, &flags);
    }

    flags = &project->base.linker.flags;
    array_push(&linker_flags, &flags);

    array_reverse(&linker_flags);
    linker_flags_collect(&cmd, &linker_flags);
    array_free(&linker_flags);

    result = system(string_unpack(&cmd));
    if (result < 0) {
        log_fatal("Expected return value of compiler to be non-negative.");
    }

    string_free(&cmd);
    return result;
}

void incremental_mark_changed_files(bld_project* project, bld_set* changed_files) {
    int* has_changed;
    bld_file *file, *cache_file, *temp;
    bld_iter iter;

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        bld_iter iter;
        if (file->type == BLD_DIR) {continue;}

        has_changed = set_get(changed_files, file->identifier.id);
        if (!project->base.cache.set) {
            *has_changed = 1;
            continue;
        }

        cache_file = set_get(&project->base.cache.files, file->identifier.id);

        if (has_changed == NULL) {log_fatal("File did not exist in changed_files set");}

        if (cache_file == NULL) {
            *has_changed = 1;
        } else if (file->identifier.hash != cache_file->identifier.hash) {
            *has_changed = 1;
        } else {
            continue;
        }

        iter = dependency_graph_includes_from(&project->graph, file);
        while (dependency_graph_next_file(&iter, &project->files, &temp)) {
            has_changed = set_get(changed_files, temp->identifier.id);
            if (has_changed == NULL) {log_fatal("incremental_mark_changed_files: unreachable error");}
            *has_changed = 1;
        }
    }
}


int incremental_cached_compilation(bld_project* project, bld_file* file) {
    int exists, new_options;
    bld_file* f;

    f = set_get(&project->base.cache.files, file->identifier.id);
    if (f != NULL) {
        exists = 1;
        new_options = (file->identifier.hash != f->identifier.hash);
    } else {
        exists = 0;
        new_options = 0;
    }

    if (exists && !new_options) {
        log_debug("Found \"%s\" in cache", string_unpack(&file->name));
    } else if (exists && new_options) {
        log_debug("Invalidated cache of \"%s\"", string_unpack(&file->name));
    } else {
        log_debug("Did not find \"%s\" in cache", string_unpack(&file->name));
    }

    return exists && !new_options;
}


int incremental_compile_with_absolute_path(bld_project* project, char* name) {
    bld_iter iter;
    int result = 0, any_compiled = 0, temp;
    bld_path path;
    bld_file *file;
    bld_set changed_files;

    temp = 0;
    changed_files = set_new(sizeof(int));
    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        set_add(&changed_files, file->identifier.id, &temp);
    }

    dependency_graph_extract_includes(&project->graph, &project->files);
    incremental_mark_changed_files(project, &changed_files);

    result = incremental_compile_changed_files(project, &changed_files, &any_compiled);
    set_free(&changed_files);
    if (result) {
        log_warn("Could not compile all files, no executable generated.");
        return result;
    }

    path = path_copy(&project->base.root);
    if (project->base.cache.loaded) {
        path_append_path(&path, &project->base.cache.root);
    }

    dependency_graph_extract_symbols(&project->graph, &project->files, &path);
    path_free(&path);

    if (!any_compiled) {
        log_debug("Entire project existed in cache, generating executable");
    }

    temp = incremental_link_executable(project, name);
    if (temp) {
        log_warn("Could not link final executable");
        result = temp;
    } else {
        log_info("Compiled executable: \"%s\"", name);
    }

    if (!project->base.cache.loaded) {
        bld_iter iter;
        bld_path obj_path;
        bld_string str;
        bld_file* file;
        char obj_name[FILENAME_MAX];

        iter = iter_set(&project->files);
        while (iter_next(&iter, (void**) &file)) {
            if (file->type == BLD_HEADER) {continue;}

            obj_path = path_copy(&project->base.root);
            serialize_identifier(obj_name, file);
            path_append_string(&obj_path, obj_name);
            str = obj_path.str;
            string_append_string(&str, ".o");

            remove(string_unpack(&str));
            string_free(&str);
        }
    }

    if (!any_compiled && !result) {
        return -1;
    } else {
        return result;
    }
}

int incremental_compile_changed_files(bld_project* project, bld_set* changed_files, int* any_compiled) {
    FILE* cached_file;
    int *has_changed, result, temp;
    char compiled_name[FILENAME_MAX];
    bld_iter iter;
    bld_path path;
    bld_string compiled_path;
    bld_file* file;

    result = 0;
    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        if (file->type == BLD_HEADER || file->type == BLD_DIR) {continue;}

        has_changed = set_get(changed_files, file->identifier.id);
        if (has_changed == NULL) {log_fatal("incremental_compile_with_absolute_path: internal error");}

        path = path_copy(&project->base.root);
        if (project->base.cache.loaded) {
            path_append_path(&path, &project->base.cache.root);
        }
        serialize_identifier(compiled_name, file);
        path_append_string(&path, compiled_name);

        compiled_path = path.str;
        string_append_string(&compiled_path, ".o");

        cached_file = fopen(string_unpack(&compiled_path), "r");
        if (cached_file != NULL) {fclose(cached_file);}

        string_free(&compiled_path);

        if (!*has_changed && cached_file != NULL) {continue;}

        if (!*has_changed && cached_file == NULL) {
            log_info("Cache is missing \"%s\", recompiling", string_unpack(&file->name));
        }

        *any_compiled = 1;
        *has_changed = 0;
        temp = incremental_compile_file(project, file);
        if (temp) {
            log_warn("Compiled \"%s\" with errors", string_unpack(&file->name));
            result = temp;
        }
    }

    return result;
}

int incremental_compile_project(bld_project* project, char* name) {
    int result;
    bld_path executable_path = path_copy(&project->base.root);
    path_append_string(&executable_path, name);
    result = incremental_compile_with_absolute_path(project, path_to_string(&executable_path));
    path_free(&executable_path);
    return result;
}

int incremental_test_project(bld_project* project, char* path) {
    log_fatal("incremental_test_project: not implemented.");
    (void)(project); /* Suppress unused parameter warning */
    (void)(path);
    return -1;
}
