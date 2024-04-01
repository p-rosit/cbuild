#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "incremental.h"

void    incremental_index_project(bld_project*, bld_forward_project*);
void    incremental_index_possible_file(bld_project*, bld_path*, char*);
void    incremental_index_recursive(bld_project*, bld_forward_project*, bld_path*, char*);
void    incremental_apply_main_file(bld_project*, bld_forward_project*);
void    incremental_apply_compilers(bld_project*, bld_forward_project*);

int     incremental_compile_file(bld_project*, bld_file*);
int     incremental_compile_total(bld_project*, char*);
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
    project.graph = dependency_graph_new();

    if (fproject->rebuilding) {
        char* main_name;
        bld_path main_path;
        main_path.str = fproject->main_file_name;

        main_name = path_get_last_string(&main_path);
        incremental_index_possible_file(&project, &main_path, main_name);
    }
    incremental_index_project(&project, fproject);

    incremental_apply_main_file(&project, fproject);
    incremental_apply_compilers(&project, fproject);

    hash = compiler_hash(&project.base.compiler, 5031);
    iter = iter_set(&project.files);
    while (iter_next(&iter, (void**) &file)) {
        file->identifier.hash = file_hash(file, &project.base.file_compilers, hash);
    }

    if (project.base.cache.set) {
        incremental_apply_cache(&project);
    }

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
        cached = set_get(&project->base.cache.files, file->identifier.id);
        if (cached == NULL) {continue;}

        if (file->identifier.hash != cached->identifier.hash) {
            continue;
        }

        log_debug("Found \"%s\" in cache: %lu include(s), %lu defined, %lu undefined", string_unpack(&file->name), cached->includes.size, cached->defined_symbols.size, cached->undefined_symbols.size);

        file->includes = set_copy(&cached->includes);
        graph_add_node(&project->graph.include_graph, file->identifier.id);

        if (file->type == BLD_HEADER) {continue;}
        
        file_symbols_copy(file, &cached->defined_symbols, &cached->undefined_symbols);
        graph_add_node(&project->graph.symbol_graph, file->identifier.id);
    }

    project->base.cache.applied = 1;
}


void incremental_index_possible_file(bld_project* project, bld_path* path, char* name) {
    int exists;
    char* file_ending;
    bld_path file_path;
    bld_file file;

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
    if (exists) {file_free(&file);}
}

void incremental_index_recursive(bld_project* project, bld_forward_project* forward_project, bld_path* path, char* name) {
    char* str_path;
    bld_path sub_path;
    DIR* dir;
    struct dirent* file_ptr;

    str_path = path_to_string(path);
    dir = opendir(str_path);
    if (dir == NULL) {
        incremental_index_possible_file(project, path, name);
        return;
    }

    if (name == NULL) {
        log_debug("Searching under root: \"%s\"", path_to_string(path));
    } else {
        log_debug("Searching under: \"%s\": named \"%s\"", path_to_string(path), name);
    }

    while ((file_ptr = readdir(dir)) != NULL) {
        if (strcmp(file_ptr->d_name, ".") == 0 || strcmp(file_ptr->d_name, "..") == 0) {
            continue;
        }
        if (file_ptr->d_name[0] == '.') {
            continue;
        }
        if (set_has(&forward_project->ignore_paths, file_ptr->d_ino)) {
            log_debug("Ignoring: \"%s" BLD_PATH_SEP "%s\"", path_to_string(path), file_ptr->d_name);
            continue;
        }

        sub_path = path_copy(path);
        path_append_string(&sub_path, file_ptr->d_name);
        incremental_index_recursive(project, forward_project, &sub_path, file_ptr->d_name);
        path_free(&sub_path);
    }
    
    closedir(dir);
}

void incremental_index_project(bld_project* project, bld_forward_project* forward_project) {
    DIR* dir;
    char* name;
    bld_path *path, extra_path;
    bld_iter iter;

    dir = opendir(path_to_string(&forward_project->base.root));
    if (dir == NULL) {log_fatal("Could not open project root \"%s\"", path_to_string(&forward_project->base.root));}
    closedir(dir);

    log_info("Indexing project under root");
    incremental_index_recursive(project, forward_project, &forward_project->base.root, NULL);

    iter = iter_array(&forward_project->extra_paths);
    while (iter_next(&iter, (void**) &path)) {
        extra_path = path_copy(&project->base.root);
        path_append_path(&extra_path, path);
        name = path_get_last_string(&extra_path);
        log_info("Indexing files under \"%s\"", path_to_string(&extra_path));

        incremental_index_recursive(project, forward_project, &extra_path, name);
        path_free(&extra_path);
    }
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
    int has_file, has_compiler;
    size_t index;
    bld_iter file_iter, compiler_iter;
    bld_string* file_name;
    bld_compiler* compiler;

    file_iter = iter_array(&fproject->file_names);
    compiler_iter = iter_array(&fproject->base.file_compilers);

    index = 0;
    while (1) {
        int match_found;
        bld_iter iter;
        bld_file* file;
        bld_path path;

        has_file = iter_next(&file_iter, (void**) &file_name);
        has_compiler = iter_next(&compiler_iter, (void**) &compiler);

        if (has_file != has_compiler) {
            log_fatal("incremental_apply_compilers: internal error, there is not an equal amount of files and compilers");
        }
        if (!has_file && !has_compiler) {break;}

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

int incremental_compile_file(bld_project* project, bld_file* file) {
    int result;
    char name[FILENAME_MAX], **flag;
    bld_iter iter;
    bld_compiler* compiler;
    bld_string cmd = string_new();
    bld_path path;

    if (file->compiler > 0) {
        compiler = array_get(&project->base.file_compilers, file->compiler);
    } else {
        compiler = &project->base.compiler;
    }

    string_append_string(&cmd, compiler->executable);
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
        string_append_string(&cmd, *flag);
    }

    result = system(string_unpack(&cmd));
    string_free(&cmd);
    return result;
}

int incremental_compile_total(bld_project* project, char* executable_name) {
    int result;
    char name[FILENAME_MAX], **flag;
    bld_path path;
    bld_compiler compiler = project->base.compiler;
    bld_file *main_file, *file;
    bld_string cmd;
    bld_iter iter;

    main_file = set_get(&project->files, project->main_file);
    if (main_file == NULL) {
        log_fatal("No main file has been set");
        return 1;
    }

    cmd = string_new();
    string_append_string(&cmd, compiler.executable);
    string_append_space(&cmd);

    string_append_string(&cmd, "-o ");
    string_append_string(&cmd, executable_name);
    string_append_space(&cmd);

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
    }

    iter = iter_array(&compiler.flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_append_space(&cmd);
        string_append_string(&cmd, *flag);
    }

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

    temp = incremental_compile_total(project, name);
    if (temp) {
        log_warn("Could not compile final executable");
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
        if (file->type == BLD_HEADER) {continue;}

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
