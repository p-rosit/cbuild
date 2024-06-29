#include <stdio.h>
#include <string.h>
#include "os.h"
#include "logging.h"
#include "incremental.h"

void    incremental_make_root(bld_project*, bld_forward_project*);
void    incremental_index_project(bld_project*, bld_forward_project*);
void    incremental_index_possible_file(bld_project*, uintmax_t, bld_path*, bld_path*, char*);
void    incremental_index_recursive(bld_project*, bld_forward_project*, uintmax_t, bld_path*, bld_path*, char*, int);
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
    bld_iter iter;
    bld_file* file;

    project.base = fproject->base;
    project.files = set_new(sizeof(bld_file));
    project.graph = dependency_graph_new();

    incremental_make_root(&project, fproject);

    if (fproject->base.rebuilding) {
        char* main_name;
        bld_path temp;
        bld_path main_path;
        bld_path main_relative_path;

        temp = path_from_string(string_unpack(&fproject->main_file_name));
        main_name = path_get_last_string(&temp);

        main_path = path_copy(&fproject->base.build_of->root);
        path_append_string(&main_path, main_name);
        main_relative_path = path_from_string(main_name);

        incremental_index_possible_file(&project, project.root_dir, &main_path, &main_relative_path, main_name);

        path_free(&main_path);
        path_free(&temp);
    }

    incremental_index_project(&project, fproject);

    incremental_apply_main_file(&project, fproject);
    incremental_apply_compilers(&project, fproject);
    incremental_apply_linker_flags(&project, fproject);

    {
        bld_file* root;

        root = set_get(&project.files, project.root_dir);
        if (root == NULL) {
            log_fatal(LOG_FATAL_PREFIX "internal error");
        }

        file_determine_all_languages_under(root, &project.files);
    }

    iter = iter_set(&project.files);
    while (iter_next(&iter, (void**) &file)) {
        file->identifier.hash = file_hash(file, &project.files);
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
        if (file->type == BLD_FILE_DIRECTORY) {continue;}

        cached = set_get(&project->base.cache.files, file->identifier.id);
        if (cached == NULL) {continue;}

        if (file->identifier.hash != cached->identifier.hash) {continue;}
        file->compile_successful = 1;

        file_includes_copy(file, cached);

        switch (file->type) {
            case (BLD_FILE_IMPLEMENTATION): {
                log_debug("Found \"%s\" in cache: %lu include(s), %lu undefined, %lu defined", string_unpack(&file->name), cached->info.impl.includes.size, cached->info.impl.undefined_symbols.size, cached->info.impl.defined_symbols.size);
            } break;
            case (BLD_FILE_INTERFACE): {
                log_debug("Found \"%s\" in cache: %lu include(s)", string_unpack(&file->name), cached->info.header.includes.size);
            } break;
            case (BLD_FILE_TEST): {
                log_debug("Found \"%s\" in cache: %lu include(s), %lu undefined", string_unpack(&file->name), cached->info.test.includes.size, cached->info.test.undefined_symbols.size);
            } break;
            default: {log_fatal("incremental_apply_cache: unrecognized file type, unreachable error");}
        }

        graph_add_node(&project->graph.include_graph, file->identifier.id);

        if (file->type == BLD_FILE_INTERFACE) {continue;}

        file_symbols_copy(file, cached);
        graph_add_node(&project->graph.symbol_graph, file->identifier.id);
    }

    project->base.cache.applied = 1;
}


void incremental_index_possible_file(bld_project* project, uintmax_t parent_id, bld_path* path, bld_path* relative_path, char* name) {
    int exists;
    char* file_ending;
    bld_file file, *parent, *temp;
    bld_string packed_name;

    file_ending = strrchr(name, '.');
    if (file_ending == NULL) {
        path_free(relative_path);
        return;
    }

    packed_name = string_pack(name);
    if (compiler_file_is_implementation(&project->base.compiler_handles, &packed_name)) {
        if (strncmp(name, "test", 4) == 0) {
            file = file_test_new(path, relative_path, name);
        } else {
            file = file_implementation_new(path, relative_path, name);
        }
    } else if (compiler_file_is_header(&project->base.compiler_handles, &packed_name)) {
        file = file_interface_new(path, relative_path, name);
    } else {
        path_free(relative_path);
        return;
    }

    exists = set_add(&project->files, file.identifier.id, &file);
    if (exists) {
        log_error("encountered \"%s\" multiple times while indexing", string_unpack(&file.name));
        file_free(&file);
    }

    parent = set_get(&project->files, parent_id);
    if (parent == NULL) {log_fatal("incremental_index_possible_file: internal error");}
    temp = set_get(&project->files, file.identifier.id);
    if (temp == NULL) {log_fatal("incremental_index_possible_file: internal temp error");}
    file_dir_add_file(parent, temp);
}

void incremental_index_recursive(bld_project* project, bld_forward_project* forward_project, uintmax_t parent_id, bld_path* path, bld_path* relative_path, char* name, int adding_files) {
    char *file_name;
    uintmax_t directory_id;
    bld_path new_path;
    bld_os_dir* dir;
    bld_os_file* file_ptr;

    dir = os_dir_open(path_to_string(path));
    if (dir == NULL && adding_files) {
        incremental_index_possible_file(project, parent_id, path, relative_path, name);
        return;
    } else if (dir == NULL && !adding_files) {
        path_free(relative_path);
        return;
    }

    if (name == NULL) {
        log_debug("Searching under root: \"%s\"", path_to_string(path));
    } else {
        log_debug("Searching under: \"%s\": named \"%s\"", path_to_string(path), name);
    }

    if (relative_path == NULL) {
        new_path = path_from_string(".");
    } else if (name != NULL) {
        new_path = *relative_path;
    } else {
        log_fatal(LOG_FATAL_PREFIX "unreachable error");
    }

    if (name != NULL) {
        int exists;
        bld_file directory;
        bld_file* parent;
        bld_file* temp;

        directory = file_directory_new(path, &new_path, name);
        exists = set_add(&project->files, directory.identifier.id, &directory);
        if (exists) {
            log_fatal(LOG_FATAL_PREFIX "encountered \"%s\" multiple times while indexing", string_unpack(&directory.name));
            return; /* unreachable */
        }

        temp = set_get(&project->files, directory.identifier.id);
        if (temp == NULL) {log_fatal("incremental_index_recursive: internal temp error");}
        parent = set_get(&project->files, parent_id);
        if (parent == NULL) {log_fatal("incremental_index_recursive: internal error");}

        directory_id = directory.identifier.id;
        file_dir_add_file(parent, temp);
    } else {
        directory_id = parent_id;
    }

    if (adding_files) {
        if (set_has(&forward_project->ignore_paths, directory_id)) {
            log_debug("Ignoring files under: \"%s\"", path_to_string(path));
            adding_files = 0;
        }
    } else {
        if (set_has(&forward_project->extra_paths, directory_id)) {
            log_debug("Adding files under: \"%s\"", path_to_string(path));
            adding_files = 1;
        }
    }

    while ((file_ptr = os_dir_read(dir)) != NULL) {
        bld_path sub_path;
        bld_path temp;

        file_name = os_file_name(file_ptr);
        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) {
            continue;
        }
        if (file_name[0] == '.') {
            continue;
        }

        sub_path = path_copy(path);
        path_append_string(&sub_path, file_name);
        temp = path_copy(&new_path);
        path_append_string(&temp, file_name);

        incremental_index_recursive(project, forward_project, directory_id, &sub_path, &temp, file_name, adding_files);

        path_free(&sub_path);
    }
    
    if (name == NULL) {
        path_free(&new_path);
    }
    os_dir_close(dir);
}

void incremental_index_project(bld_project* project, bld_forward_project* forward_project) {
    bld_os_dir* dir;
    bld_path path;

    path = path_copy(&project->base.root);
    dir = os_dir_open(path_to_string(&path));
    if (dir == NULL) {log_fatal("Could not open project root \"%s\"", path_to_string(&path));}
    os_dir_close(dir);

    log_info("Indexing project under root");
    incremental_index_recursive(project, forward_project, project->root_dir, &path, NULL, NULL, 1);

    path_free(&path);
}

void incremental_make_root(bld_project* project, bld_forward_project* fproject) {
    int exists;
    bld_path path;
    bld_path relative;
    bld_file root;

    path = path_copy(&project->base.root);
    relative = path_from_string(".");
    root = file_directory_new(&path, &relative, ".");
    path_free(&path);

    root.build_info.compiler_set = 1;
    root.build_info.compiler.type = BLD_COMPILER;
    root.build_info.compiler.as.compiler = fproject->compiler;

    project->root_dir = root.identifier.id;
    exists = set_add(&project->files, root.identifier.id, &root);

    if (exists) {log_fatal("incremental_make_root: root has already been initialized, has the project already been resolved?");}
}

void incremental_apply_main_file(bld_project* project, bld_forward_project* fproject) {
    int match_found;
    bld_iter iter;
    bld_path path;
    bld_file* file;

    match_found = 0;
    path.str = fproject->main_file_name;

    if (fproject->base.rebuilding) {
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
    bld_iter name_iter, compiler_iter;
    bld_string* file_name;
    bld_compiler_or_flags* compiler;

    if (fproject->compiler_file_names.size != fproject->file_compilers.size) {
        log_fatal("incremental_apply_compilers: internal error, there is not an equal amount of files and compilers");
    }

    index = 0;
    name_iter = iter_array(&fproject->compiler_file_names);
    compiler_iter = iter_array(&fproject->file_compilers);
    while (iter_next(&name_iter, (void**) &file_name) && iter_next(&compiler_iter, (void**) &compiler)) {
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
                file->build_info.compiler_set = 1;
                file->build_info.compiler = *compiler;
            }
        }

        path_free(&path);

        index++;
    }
}

void incremental_apply_linker_flags(bld_project* project, bld_forward_project* fproject) {
    size_t index;
    bld_iter name_iter, linker_flags_iter;
    bld_string* file_name;
    bld_linker_flags* flags;

    if (fproject->linker_flags_file_names.size != fproject->file_linker_flags.size) {
        log_fatal("incremental_apply_linker_flags: internal error, there is not an equal amount of files and linker flags");
    }

    index = 0;
    name_iter = iter_array(&fproject->linker_flags_file_names);
    linker_flags_iter = iter_array(&fproject->file_linker_flags);
    while (iter_next(&name_iter, (void**) &file_name) && iter_next(&linker_flags_iter, (void**) &flags)) {
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
                    log_fatal("Applying linker flags to \"%s\" but several matches were found, specify more of path to determine exact match", string_unpack(file_name));
                }
                match_found = 1;
                file->build_info.linker_set = 1;
                file->build_info.linker_flags = *flags;
            }
        }

        path_free(&path);

        index++;
    }
}

int incremental_compile_file(bld_project* project, bld_file* file) {
    int result;
    bld_string cmd, object_name;
    bld_compiler* compiler;
    bld_path path;
    bld_array compiler_flags;
    log_info("Compiling: \"%s\"", string_unpack(&file->name));

    file_assemble_compiler(file, &project->files, &compiler, &compiler_flags);

    cmd = string_new();
    string_append_string(&cmd, string_unpack(&compiler->executable));
    compiler_flags_expand(&cmd, &compiler_flags);
    string_append_space(&cmd);

    if (!project->base.rebuilding || file->identifier.id != project->main_file) {
        path = path_copy(&project->base.root);
    } else if (file->identifier.id == project->main_file) {
        path = path_copy(&project->base.build_of->root);
    }
    path_append_path(&path, &file->path);
    string_append_string(&cmd, path_to_string(&path));
    path_free(&path);

    path = path_copy(&project->base.root);
    if (project->base.cache.loaded) {
        path_append_path(&path, &project->base.cache.root);
    }

    object_name = file_object_name(file);
    string_append_string(&object_name, ".o");

    result = compile_to_object(compiler->type, &cmd, &path, &object_name);

    path_free(&path);
    string_free(&object_name);
    string_free(&cmd);
    array_free(&compiler_flags);
    return result;
}

int incremental_link_executable(bld_project* project, char* executable_name) {
    int result;
    bld_path path;
    bld_file *main_file, *file;
    bld_string cmd;
    bld_array linker_flags;
    bld_iter iter;

    main_file = set_get(&project->files, project->main_file);
    if (main_file == NULL) {
        log_fatal(LOG_FATAL_PREFIX "No main file has been set");
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
        bld_string object_name;
        bld_array file_flags;
        string_append_space(&cmd);

        path = path_copy(&project->base.root);
        if (project->base.cache.loaded) {
            path_append_path(&path, &project->base.cache.root);
        }
        object_name = file_object_name(file);
        path_append_string(&path, string_unpack(&object_name));
        
        string_append_string(&cmd, path_to_string(&path));
        string_append_string(&cmd, ".o");
        path_free(&path);

        file_assemble_linker_flags(file, &project->files, &file_flags);
        linker_flags_expand(&cmd, &file_flags);

        array_free(&file_flags);
        string_free(&object_name);
    }

    linker_flags_append(&cmd, &project->base.linker.flags);
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
        if (file->type == BLD_FILE_DIRECTORY) {continue;}

        has_changed = set_get(changed_files, file->identifier.id);
        if (has_changed == NULL) {log_fatal("File did not exist in changed_files set");}
        if (!project->base.cache.set) {
            *has_changed = 1;
            continue;
        }

        cache_file = set_get(&project->base.cache.files, file->identifier.id);

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
    int result, any_compiled, temp;
    bld_path path;
    bld_file *file;
    bld_set changed_files;

    temp = 0;
    changed_files = set_new(sizeof(int));
    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        set_add(&changed_files, file->identifier.id, &temp);
    }

    dependency_graph_extract_includes(&project->graph, &project->base, project->main_file, &project->files);
    incremental_mark_changed_files(project, &changed_files);

    any_compiled = 0;
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

        iter = iter_set(&project->files);
        while (iter_next(&iter, (void**) &file)) {
            bld_string object_name;

            if (file->type == BLD_FILE_INTERFACE) {continue;}

            obj_path = path_copy(&project->base.root);
            object_name = file_object_name(file);
            path_append_string(&obj_path, string_unpack(&object_name));
            str = obj_path.str;
            string_append_string(&str, ".o");

            remove(string_unpack(&str));
            string_free(&str);
            string_free(&object_name);
        }
    }

    if (!any_compiled && !result) {
        return -1;
    } else {
        return result;
    }
}

int incremental_compile_changed_files(bld_project* project, bld_set* changed_files, int* any_compiled) {
    int result;
    bld_iter iter;
    bld_file* file;

    result = 0;
    iter = iter_set(&project->files);
    while (iter_next(&iter, (void**) &file)) {
        int *has_changed, temp;
        FILE* cached_file;
        bld_string object_name;
        bld_string compiled_path;
        bld_path path;

        if (file->type == BLD_FILE_INTERFACE || file->type == BLD_FILE_DIRECTORY) {continue;}

        has_changed = set_get(changed_files, file->identifier.id);
        if (has_changed == NULL) {log_fatal("incremental_compile_with_absolute_path: internal error");}

        path = path_copy(&project->base.root);
        if (project->base.cache.loaded) {
            path_append_path(&path, &project->base.cache.root);
        }
        object_name = file_object_name(file);
        path_append_string(&path, string_unpack(&object_name));

        compiled_path = path.str;
        string_append_string(&compiled_path, ".o");

        cached_file = fopen(string_unpack(&compiled_path), "r");
        if (cached_file != NULL) {fclose(cached_file);}

        string_free(&object_name);
        string_free(&compiled_path);

        if (!*has_changed && cached_file != NULL) {continue;}

        if (!*has_changed && cached_file == NULL) {
            log_info("Cache is missing \"%s\", recompiling", string_unpack(&file->name));
        }

        *any_compiled = 1;
        *has_changed = 0;
        temp = incremental_compile_file(project, file);
        if (!temp) {
            file->compile_successful = 1;
        } else {
            log_warn("Compiled \"%s\" with errors", string_unpack(&file->name));
            file->compile_successful = 0;
            result = temp;
        }
    }

    return result;
}

int incremental_compile_project(bld_project* project, char* name) {
    int result;
    bld_path executable_path;

    if (project->base.rebuilding) {
        executable_path = path_copy(&project->base.build_of->root);
    } else {
        executable_path = path_copy(&project->base.root);
    }
    path_append_string(&executable_path, name);

    result = incremental_compile_with_absolute_path(project, path_to_string(&executable_path));

    path_free(&executable_path);
    return result;
}
