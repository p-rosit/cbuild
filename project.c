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
    bld_string str = new_string();
    char* name;
    append_string(&str, get_last_dir(root));

    make_string(&str);
    if (strncmp(str.chars, "old_", 4) == 0) {
        name = str.chars + 4;
    } else {
        name = str.chars;
    }

    build_path = path_from_string(name);
    remove_file_ending(&build_path);
    append_string(&build_path.str, ".c");

    free_string(&str);
    return build_path;
}

bld_project make_project(bld_path root, bld_compiler compiler) {
    return (bld_project) {
        .ignore_root = 0,
        .root = root,
        .build = new_path(),
        .extra_paths = new_paths(),
        .ignore_paths = new_ignore_ids(),
        .main_file = 0,
        .compiler = compiler,
        .files = new_files(),
        .graph = new_graph(NULL),
        .cache = NULL,
    };
}

bld_project new_project(bld_path root, bld_compiler compiler) {
    FILE* f;
    bld_path build_file_path;
    bld_project project;

    build_file_path = extract_build_path(&root);
    
    remove_last_dir(&root);
    project = make_project(root, compiler);

    f = fopen(path_to_string(&build_file_path), "r");
    if (f == NULL) {
        log_fatal("Expected executable to have same name (and be in same directory) as build file. Could not find \"%s\"", path_to_string(&build_file_path));
    }
    fclose(f);

    ignore_path(&project, path_to_string(&build_file_path));
    free_path(&build_file_path);

    return project;
}

void free_cache(bld_project* cache) {
    if (cache == NULL) {return;}
    free_path(&cache->root);
    free_path(&cache->build);
    free_compiler(&cache->compiler);
    free_graph(&cache->graph);
    free_files(&cache->files);
    free(cache);
}

void free_project(bld_project* project) {
    if (project == NULL) {return;}
    free_path(&project->root);
    free_path(&project->build);
    free_paths(&project->extra_paths);
    free_ignore_ids(&project->ignore_paths);
    free_compiler(&project->compiler);
    free_graph(&project->graph);
    free_files(&project->files);
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

    iter = bld_iter_set(&files.set, sizeof(bld_file));
    while (bld_set_next(&iter, (void**) &file)) {
        if (path_ends_with(&file->path, &path)) {
            if (match_found) {
                log_fatal("Name of file \"%s\" is ambiguous, found several matches.", str);
            }
            match_found = 1;
            file->compiler = c_ptr;
        }
    }

    free_path(&path);
    if (!match_found) {
        log_fatal("No file matching \"%s\" could be found.", str);
    }
}

void set_main_file(bld_project* project, char* str) {
    int match_found = 0;
    bld_path path = path_from_string(str);
    bld_files files = project->files;
    bld_file* file;
    bld_iter iter = bld_iter_set(&files.set, sizeof(bld_file));

    while (bld_set_next(&iter, (void**) &file)) {
        if (path_ends_with(&file->path, &path)) {
            if (match_found) {
                log_fatal("Name of main file \"%s\" is ambiguous, found several matches.", str);
            }
            match_found = 1;
            project->main_file = file->identifier.id;
        }
    }

    free_path(&path);
    if (!match_found) {
        log_fatal("No file matching \"%s\" could be found.", str);
    }
}

void add_build(bld_project* project, char* path) {
    free_path(&project->build);
    project->build = path_from_string(path);
    ignore_path(project, path);
}

uintmax_t test_open(bld_path* path) {
    bld_stat file_stat;
    
    if (stat(path_to_string(path), &file_stat) < 0) {
        log_fatal("\"%s\" is not a valid path under the root", path_to_string(path));
    }

    return file_stat.st_ino;
}

void add_path(bld_project* project, char* path) {
    bld_path test = copy_path(&project->root);
    append_dir(&test, path);
    test_open(&test);

    push_path(&project->extra_paths, path_from_string(path));

    free_path(&test);
}

void ignore_path(bld_project* project, char* path) {
    bld_path test = copy_path(&project->root);
    append_dir(&test, path);

    append_ignore_id(&project->ignore_paths, test_open(&test));

    free_path(&test);
}

void ignore_root(bld_project* project) {
    project->ignore_root = 1;
}

void index_possible_file(bld_project* project, bld_path* path, char* name) {
    int exists;
    char* file_ending;
    bld_path file_path;
    bld_file file;
    bld_stat file_stat;
    // log_warn("Indexing: \"%s\", name \"%s\"", path_to_string(path), name);

    file_ending = strrchr(name, '.');
    if (file_ending == NULL) {return;}

    if (stat(path_to_string(path), &file_stat) < 0) {
        log_fatal("Could not extract information about \"%s\"", path_to_string(path));
    }

    file_path = copy_path(path);
    if (strncmp(name, "test", 4) == 0 && strcmp(file_ending, ".c") == 0) {
        file = make_test(&file_path, name, &file_stat);
    } else if (strcmp(file_ending, ".c") == 0) {
        file = make_impl(&file_path, name, &file_stat);
    } else if (strcmp(file_ending, ".h") == 0) {
        file = make_header(&file_path, name, &file_stat);
    } else {
        free_path(&file_path);
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

        sub_path = copy_path(path);
        append_dir(&sub_path, file_ptr->d_name);
        index_recursive(project, &sub_path, file_ptr->d_name);
        free_path(&sub_path);
    }
    
    closedir(dir);
}

void index_project(bld_project* project) {
    bld_path *paths, extra_path;
    char* name;
    DIR* dir;

    dir = opendir(path_to_string(&project->root));
    if (dir == NULL) {log_fatal("Could not open project root \"%s\"", path_to_string(&project->root));}
    closedir(dir);

    if (!project->ignore_root) {
        log_info("Indexing project under root");
        index_recursive(project, &project->root, NULL);
    } else {
        log_info("Ignoring root");
    }

    paths = project->extra_paths.array.values;
    for (size_t i = 0; i < project->extra_paths.array.size; i++) {
        extra_path = copy_path(&project->root);
        append_path(&extra_path, &paths[i]);
        name = get_last_dir(&extra_path);
        log_info("Indexing files under \"%s\"", path_to_string(&extra_path));

        index_recursive(project, &extra_path, name);
        free_path(&extra_path);
    }

}

int compile_file(bld_project* project, bld_file* file) {
    int result;
    char name[256], **flags;
    bld_compiler compiler;
    bld_string cmd = new_string();
    bld_path path;

    if (file->compiler != NULL) {
        compiler = *(file->compiler);
    } else {
        compiler = project->compiler;
    }

    append_string(&cmd, compiler.executable);
    append_space(&cmd);

    flags = compiler.options.array.values;
    for (size_t i = 0; i < compiler.options.array.size; i++) {
        append_string(&cmd, flags[i]);
        append_space(&cmd);
    }

    append_string(&cmd, "-c ");
    append_string(&cmd, path_to_string(&file->path));

    append_string(&cmd, " -o ");

    path = copy_path(&project->root);
    append_path(&path, &(*project->cache).root);
    serialize_identifier(name, file);
    append_dir(&path, name);
    append_string(&cmd, path_to_string(&path));

    append_string(&cmd, ".o");
    free_path(&path);

    // log_warn("File: \"%s\"", make_string(&cmd));
    result = system(make_string(&cmd));
    free_string(&cmd);
    return result;
}

int compile_total(bld_project* project, char* executable_name) {
    int result;
    char name[256], **flags;
    bld_path path;
    bld_compiler compiler = project->compiler;
    bld_file *main_file, *file;
    bld_string cmd;
    bld_search_info* bfs;

    main_file = bld_set_get(&project->files.set, project->main_file, sizeof(bld_file));
    if (main_file == NULL) {
        log_fatal("No main file has been set");
        return 1;
    }

    cmd = new_string();
    append_string(&cmd, compiler.executable);
    append_space(&cmd);

    flags = compiler.options.array.values;
    for (size_t i = 0; i < compiler.options.array.size; i++) {
        append_string(&cmd, flags[i]);
        append_space(&cmd);
    }

    append_string(&cmd, "-o ");
    append_string(&cmd, executable_name);
    append_space(&cmd);

    bfs = graph_dfs_from(&project->graph, main_file);
    while (next_file(bfs, &file)) {
        path = copy_path(&project->root);
        append_path(&path, &(*project->cache).root);
        serialize_identifier(name, file);
        append_dir(&path, name);
        
        append_string(&cmd, path_to_string(&path));
        append_string(&cmd, ".o ");
        free_path(&path);
    }

    // log_warn("Final: \"%s\"", make_string(&cmd));
    result = system(make_string(&cmd));
    if (result < 0) {
        log_fatal("Expected return value of compiler to be non-negative.");
    }

    free_string(&cmd);
    return result;
}

int compile_with_absolute_path(bld_project* project, char* name) {
    int result = 0, any_compiled = 0, temp;
    uintmax_t hash;
    bld_path path;
    bld_file* file;
    bld_iter iter = bld_iter_set(&project->files.set, sizeof(bld_file));

    hash = hash_compiler(&project->compiler, 5031);
    while (bld_set_next(&iter, (void**) &file)) {
        if (file->type == BLD_HEADER) {continue;}

        file->identifier.hash = hash_file(file, hash);
        if (cached_compilation(project, file)) {
            continue;
        }

        any_compiled = 1;
        temp = compile_file(project, file);
        if (temp) {
            log_warn("Compiled \"%s\" with errors", make_string(&file->name));
            result = temp;
        }
    }

    if (result) {
        log_warn("Could not compile all files, no executable generated.");
        return result;
    }

    path = copy_path(&project->root);
    append_path(&path, &(*project->cache).root);

    free_graph(&project->graph);
    project->graph = new_graph(&project->files);

    /* TODO: move to separate function */
    generate_graph(&project->graph, &path);
    free_path(&path);

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
    bld_path executable_path = copy_path(&project->root);
    append_dir(&executable_path, name);
    result = compile_with_absolute_path(project, path_to_string(&executable_path));
    free_path(&executable_path);
    return result;
}

int cached_compilation(bld_project* project, bld_file* file) {
    int exists, new_options;
    bld_file* f;

    f = bld_set_get(&project->cache->files.set, file->identifier.id, sizeof(bld_file));
    if (f != NULL) {
        exists = 1;
        new_options = (file->identifier.hash != f->identifier.hash);
    } else {
        exists = 0;
        new_options = 0;
    }

    if (exists && !new_options) {
        log_debug("Found \"%s\" in cache", make_string(&file->name));
    } else if (exists && new_options) {
        log_debug("Invalidated cache of \"%s\"", make_string(&file->name));
    } else {
        log_debug("Did not find \"%s\" in cache", make_string(&file->name));
    }

    return exists && !new_options;
}

int test_project(bld_project* project, char* path) {
    log_warn("test_project: not implemented.");
    (void)(project); /* Suppress unused parameter warning */
    (void)(path);
    return -1;
}

bld_ignore new_ignore_ids() {
    return (bld_ignore) {.set = bld_set_new()};
}

void free_ignore_ids(bld_ignore* ignore) {
    bld_set_free(&ignore->set);
}

void append_ignore_id(bld_ignore* ignore, uintmax_t id) {
    bld_set_add(&ignore->set, id, NULL, 0);
}
