#include <string.h>
#include "logging.h"
#include "incremental.h"


void index_possible_file(bld_project* project, bld_path* path, char* name) {
    int exists;
    char* file_ending;
    bld_path file_path;
    bld_file file;
    // log_warn("Indexing: \"%s\", name \"%s\"", path_to_string(path), name);

    file_ending = strrchr(name, '.');
    if (file_ending == NULL) {return;}

    file_path = path_copy(path);
    if (strncmp(name, "test", 4) == 0 && strcmp(file_ending, ".c") == 0) {
        file = make_test(&file_path, name);
    } else if (strcmp(file_ending, ".c") == 0) {
        file = make_impl(&file_path, name);
    } else if (strcmp(file_ending, ".h") == 0) {
        file = make_header(&file_path, name);
    } else {
        path_free(&file_path);
        return;
    }

    exists = append_file(&project->files, file);
    if (exists) {free_file(&file);}
}

void index_recursive(bld_project* project, bld_path* path, char* name) {
    char* str_path;
    bld_path sub_path;
    DIR* dir;
    bld_dirent* file_ptr;

    str_path = path_to_string(path);
    dir = opendir(str_path);
    if (dir == NULL) {
        index_possible_file(project, path, name);
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
        if (bld_set_has(&project->ignore_paths.set, file_ptr->d_ino)) {
            log_debug("Ignoring: \"%s" BLD_PATH_SEP "%s\"", path_to_string(path), file_ptr->d_name);
            continue;
        }

        sub_path = path_copy(path);
        path_append_string(&sub_path, file_ptr->d_name);
        index_recursive(project, &sub_path, file_ptr->d_name);
        path_free(&sub_path);
    }
    
    closedir(dir);
}
void index_project(bld_project* project) {
    bld_iter iter;
    bld_path *path, extra_path;
    char* name;
    DIR* dir;

    dir = opendir(path_to_string(&project->root));
    if (dir == NULL) {log_fatal("Could not open project root \"%s\"", path_to_string(&project->root));}
    closedir(dir);

    log_info("Indexing project under root");
    index_recursive(project, &project->root, NULL);

    iter = bld_iter_array(&project->extra_paths);
    while (bld_array_next(&iter, (void**) &path)) {
        extra_path = path_copy(&project->root);
        path_append_path(&extra_path, path);
        name = path_get_last_string(&extra_path);
        log_info("Indexing files under \"%s\"", path_to_string(&extra_path));

        index_recursive(project, &extra_path, name);
        path_free(&extra_path);
    }
}

int compile_file(bld_project* project, bld_file* file) {
    int result;
    char name[256], **flag;
    bld_iter iter;
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

    iter = bld_iter_array(&compiler.options);
    while (bld_array_next(&iter, (void**) &flag)) {
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

int compile_total(bld_project* project, char* executable_name) {
    int result;
    char name[256], **flag;
    bld_iter iter;
    bld_path path;
    bld_compiler compiler = project->compiler;
    bld_file *main_file, *file;
    bld_string cmd;
    bld_search_info* bfs;

    main_file = bld_set_get(&project->files.set, project->main_file);
    if (main_file == NULL) {
        log_fatal("No main file has been set");
        return 1;
    }

    cmd = string_new();
    string_append_string(&cmd, compiler.executable);
    string_append_space(&cmd);

    iter = bld_iter_array(&compiler.options);
    while (bld_array_next(&iter, (void**) &flag)) {
        string_append_string(&cmd, *flag);
        string_append_space(&cmd);
    }

    string_append_string(&cmd, "-o ");
    string_append_string(&cmd, executable_name);
    string_append_space(&cmd);

    bfs = graph_functions_from(&project->graph, main_file);
    while (next_file(bfs, &file)) {
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

void mark_changed_files(bld_project* project) {
    int* has_changed;
    bld_search_info* info;
    bld_iter iter;
    bld_file *file, *temp;

    iter = bld_iter_set(&project->files.set);
    while (bld_set_next(&iter, (void**) &file)) {
        has_changed = bld_set_get(&project->changed_files, file->identifier.id);
        if (has_changed == NULL) {log_fatal("mark_changed_files: unreachable error");}
        if (!*has_changed) {continue;}

        info = graph_includes_from(&project->graph, file);
        while (next_file(info, &temp)) {
            has_changed = bld_set_get(&project->changed_files, temp->identifier.id);
            if (has_changed == NULL) {log_fatal("mark_changed_files: unreachable error");}
            *has_changed = 1;
        }
    }
}


int cached_compilation(bld_project* project, bld_file* file) {
    int exists, new_options;
    bld_file* f;

    f = bld_set_get(&project->cache->files.set, file->identifier.id);
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


int compile_with_absolute_path(bld_project* project, char* name) {
    int result = 0, any_compiled = 0, temp, *has_changed;
    uintmax_t hash;
    bld_path path;
    bld_file *file, *cache_file;
    bld_iter iter;

    temp = 0;
    iter = bld_iter_set(&project->files.set);
    while (bld_set_next(&iter, (void**) &file)) {
        bld_set_add(&project->changed_files, file->identifier.id, &temp);
    }

    hash = hash_compiler(&project->compiler, 5031);
    iter = bld_iter_set(&project->files.set);
    while (bld_set_next(&iter, (void**) &file)) {
        file->identifier.hash = hash_file(file, hash);

        if (file->type == BLD_HEADER) {
            cache_file = bld_set_get(&project->cache->files.set, file->identifier.id);
            if (cache_file == NULL) {
                has_changed = bld_set_get(&project->changed_files, file->identifier.id);
                *has_changed = 1;
            } else if (file->identifier.hash != cache_file->identifier.hash) {
                has_changed = bld_set_get(&project->changed_files, file->identifier.id);
                *has_changed = 1;
            }
            continue;
        }

        if (cached_compilation(project, file)) {
            continue;
        }

        any_compiled = 1;
        has_changed = bld_set_get(&project->changed_files, file->identifier.id);
        *has_changed = 1;

        temp = compile_file(project, file);
        if (temp) {
            log_warn("Compiled \"%s\" with errors", string_unpack(&file->name));
            result = temp;
        }
    }

    path = path_copy(&project->root);
    path_append_path(&path, &(*project->cache).root);

    free_graph(&project->graph);
    project->graph = new_graph(&project->files);

    /* TODO: move to separate function */
    generate_graph(&project->graph, &path);
    path_free(&path);

    mark_changed_files(project);

    iter = bld_iter_set(&project->files.set);
    while (bld_set_next(&iter, (void**) &file)) {
        if (file->type == BLD_HEADER) {continue;}

        has_changed = bld_set_get(&project->changed_files, file->identifier.id);
        if (!*has_changed) {continue;}

        temp = compile_file(project, file);
        if (temp) {
            log_warn("Compiled \"%s\" with errors", string_unpack(&file->name));
            result = temp;
        }
    }

    if (result) {
        log_warn("Could not compile all files, no executable generated.");
        return result;
    }

    if (!any_compiled) {
        log_debug("Entire project existed in cache, generating executable");
    }

    temp = compile_total(project, name);
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

int compile_project(bld_project* project, char* name) {
    int result;
    bld_path executable_path = path_copy(&project->root);
    path_append_string(&executable_path, name);
    result = compile_with_absolute_path(project, path_to_string(&executable_path));
    path_free(&executable_path);
    return result;
}

int test_project(bld_project* project, char* path) {
    log_fatal("test_project: not implemented.");
    (void)(project); /* Suppress unused parameter warning */
    (void)(path);
    return -1;
}
