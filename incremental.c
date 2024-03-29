#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "incremental.h"

void    incremental_index_possible_file(bld_project*, bld_path*, char*);
void    incremental_index_recursive(bld_project*, bld_path*, char*);
int     incremental_compile_file(bld_project*, bld_file*);
int     incremental_compile_total(bld_project*, char*);
void    incremental_mark_changed_files(bld_project*);
int     incremental_cached_compilation(bld_project*, bld_file*);
int     incremental_compile_with_absolute_path(bld_project*, char*);
int     incremental_compile_changed_files(bld_project*, int*);

void incremental_apply_cache(bld_project* project) {
    bld_iter iter;
    bld_file *file, *cached;

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        cached = set_get(&project->cache->files, file->identifier.id);
        if (cached == NULL) {continue;}

        if (file->identifier.hash != cached->identifier.hash) {
            continue;
        }

        log_debug("Found \"%s\":%ju in cache: %lu include(s), %lu defined, %lu undefined", string_unpack(&file->name), cached->identifier.id, cached->includes.size, cached->defined_symbols.size, cached->undefined_symbols.size);

        file->includes = set_copy(&cached->includes);
        graph_add_node(&project->graph.include_graph, file->identifier.id);

        if (file->type == BLD_HEADER) {continue;}
        
        file_symbols_copy(file, &cached->defined_symbols, &cached->undefined_symbols);
        graph_add_node(&project->graph.symbol_graph, file->identifier.id);
    }
}


void incremental_index_possible_file(bld_project* project, bld_path* path, char* name) {
    int exists;
    char* file_ending;
    bld_path file_path;
    bld_file file;
    // log_warn("Indexing: \"%s\", name \"%s\"", path_to_string(path), name);

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

void incremental_index_recursive(bld_project* project, bld_path* path, char* name) {
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

    log_debug("Searching under: \"%s\": named \"%s\"", path_to_string(path), name);

    while ((file_ptr = readdir(dir)) != NULL) {
        if (strcmp(file_ptr->d_name, ".") == 0 || strcmp(file_ptr->d_name, "..") == 0) {
            continue;
        }
        if (file_ptr->d_name[0] == '.') {
            continue;
        }
        if (set_has(&project->ignore_paths, file_ptr->d_ino)) {
            log_debug("Ignoring: \"%s" BLD_PATH_SEP "%s\"", path_to_string(path), file_ptr->d_name);
            continue;
        }

        sub_path = path_copy(path);
        path_append_string(&sub_path, file_ptr->d_name);
        incremental_index_recursive(project, &sub_path, file_ptr->d_name);
        path_free(&sub_path);
    }
    
    closedir(dir);
}
void incremental_index_project(bld_project* project) {
    bld_path *path, extra_path;
    char* name;
    DIR* dir;

    dir = opendir(path_to_string(&project->root));
    if (dir == NULL) {log_fatal("Could not open project root \"%s\"", path_to_string(&project->root));}
    closedir(dir);

    log_info("Indexing project under root");
    incremental_index_recursive(project, &project->root, NULL);

    bld_iter iter = iter_array(&project->extra_paths);
    while (iter_next(&iter, (void**) &path)) {
        extra_path = path_copy(&project->root);
        path_append_path(&extra_path, path);
        name = path_get_last_string(&extra_path);
        log_info("Indexing files under \"%s\"", path_to_string(&extra_path));

        incremental_index_recursive(project, &extra_path, name);
        path_free(&extra_path);
    }
}

int incremental_compile_file(bld_project* project, bld_file* file) {
    int result;
    char name[FILENAME_MAX], **flag;
    bld_compiler compiler;
    bld_string cmd = string_new();
    bld_path path;

    if (file->compiler != NULL) {
        compiler = *(file->compiler);
    } else {
        compiler = project->compiler;
    }

    string_append_string(&cmd, compiler.executable);
    string_append_space(&cmd);

    bld_iter iter = iter_array(&compiler.flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_append_string(&cmd, *flag);
        string_append_space(&cmd);
    }

    string_append_string(&cmd, "-c ");
    string_append_string(&cmd, path_to_string(&file->path));

    string_append_string(&cmd, " -o ");

    path = path_copy(&project->root);
    path_append_path(&path, &(*project->cache).root);
    serialize_identifier(name, file);
    path_append_string(&path, name);
    string_append_string(&cmd, path_to_string(&path));

    string_append_string(&cmd, ".o");
    path_free(&path);

    // log_warn("File: \"%s\"", string_unpack(&cmd));
    result = system(string_unpack(&cmd));
    string_free(&cmd);
    return result;
}

int incremental_compile_total(bld_project* project, char* executable_name) {
    int result;
    char name[FILENAME_MAX], **flag;
    bld_path path;
    bld_compiler compiler = project->compiler;
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

    bld_iter iter_flags = iter_array(&compiler.flags);
    while (iter_next(&iter_flags, (void**) &flag)) {
        string_append_string(&cmd, *flag);
        string_append_space(&cmd);
    }

    string_append_string(&cmd, "-o ");
    string_append_string(&cmd, executable_name);
    string_append_space(&cmd);

    iter = dependency_graph_symbols_from(&project->graph, main_file);
    while (dependency_graph_next_file(&iter, &project->graph, &file)) {
        path = path_copy(&project->root);
        path_append_path(&path, &(*project->cache).root);
        serialize_identifier(name, file);
        path_append_string(&path, name);
        
        string_append_string(&cmd, path_to_string(&path));
        string_append_string(&cmd, ".o ");
        path_free(&path);
    }

    // log_warn("Final: \"%s\"", string_unpack(&cmd));
    result = system(string_unpack(&cmd));
    if (result < 0) {
        log_fatal("Expected return value of compiler to be non-negative.");
    }

    string_free(&cmd);
    return result;
}

void incremental_mark_changed_files(bld_project* project) {
    int* has_changed;
    bld_file *file, *cache_file, *temp;
    bld_iter iter;

    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        bld_iter iter;

        cache_file = set_get(&project->cache->files, file->identifier.id);
        has_changed = set_get(&project->changed_files, file->identifier.id);

        if (has_changed == NULL) {log_fatal("File did not exist in changed_files set");}

        if (cache_file == NULL) {
            *has_changed = 1;
        } else if (file->identifier.hash != cache_file->identifier.hash) {
            *has_changed = 1;
        } else {
            continue;
        }

        iter = dependency_graph_includes_from(&project->graph, file);
        while (dependency_graph_next_file(&iter, &project->graph, &temp)) {
            has_changed = set_get(&project->changed_files, temp->identifier.id);
            if (has_changed == NULL) {log_fatal("incremental_mark_changed_files: unreachable error");}
            *has_changed = 1;
        }
    }
}


int incremental_cached_compilation(bld_project* project, bld_file* file) {
    int exists, new_options;
    bld_file* f;

    f = set_get(&project->cache->files, file->identifier.id);
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
    uintmax_t hash;
    bld_path path;
    bld_file *file;

    temp = 0;
    hash = compiler_hash(&project->compiler, 5031);
    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        file->identifier.hash = file_hash(file, hash); // TODO: move hash computation
        set_add(&project->changed_files, file->identifier.id, &temp);
    }

    if (project->graph.files != &project->files) {
        dependency_graph_free(&project->graph);
        project->graph = dependency_graph_new(&project->files);
    }

    incremental_apply_cache(project);

    dependency_graph_extract_includes(&project->graph);
    incremental_mark_changed_files(project);

    result = incremental_compile_changed_files(project, &any_compiled);
    if (result) {
        log_warn("Could not compile all files, no executable generated.");
        return result;
    }

    path = path_copy(&project->root);
    path_append_path(&path, &(*project->cache).root);

    dependency_graph_extract_symbols(&project->graph, &path);
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

    if (!any_compiled && !result) {
        return -1;
    } else {
        return result;
    }
}

int incremental_compile_changed_files(bld_project* project, int* any_compiled) {
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

        has_changed = set_get(&project->changed_files, file->identifier.id);
        if (has_changed == NULL) {log_fatal("incremental_compile_with_absolute_path: internal error");}

        path = path_copy(&project->root);
        path_append_path(&path, &(*project->cache).root);
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
    bld_path executable_path = path_copy(&project->root);
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
