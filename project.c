#include <string.h>
#include <inttypes.h>
#include "build.h"
#include "file.h"
#include "project.h"

int         cached_compilation(bld_project*, bld_file*);

bld_ignore  new_ignore_ids();
void        append_ignore_id(bld_ignore*, uintmax_t);
void        free_ignore_ids(bld_ignore*);


bld_path extract_path(int argc, char** argv) {
    /* TODO: argv[0] is not guaranteed to contain path to executable */
    bld_path path = path_from_string(argv[0]);
    log_info("Extracted path to executable: \"%.*s\"", (int) path.str.size, path.str.chars);
    return path;
}

bld_project make_project(bld_path root, bld_compiler compiler) {
    return (bld_project) {
        .root = root,
        .build = new_path(),
        .extra_paths = new_paths(),
        .ignore_paths = new_ignore_ids(),
        .main_file = (bld_file) {.type = BLD_INVALID},
        .compiler = compiler,
        .files = new_files(),
        .graph = new_graph(NULL),
        .cache = NULL,
    };
}

bld_project new_project(bld_path root, bld_compiler compiler) {
    FILE* f;
    char* file_ending;
    bld_path build_file_path = copy_path(&root);
    bld_project project;

    remove_file_ending(&build_file_path);
    append_string(&build_file_path.str, ".c");
    
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

void free_project(bld_project* project) {
    if (project == NULL) {return;}
    free_path(&project->root);
    free_path(&project->build);
    free_paths(&project->extra_paths);
    free_ignore_ids(&project->ignore_paths);
    free_compiler(&project->compiler);
    free_graph(&project->graph);
    free_files(&project->files);
    free_project(project->cache);
    free(project->cache);
}

void set_main_file(bld_project* project, char* str) {
    int match_found = 0;
    bld_path path = path_from_string(str);
    bld_files files = project->files;

    for (size_t i = 0; i < files.size; i++) {
        if (path_ends_with(&files.files[i].path, &path)) {
            if (match_found) {
                log_fatal("Name of main file \"%s\" is ambigiuous, found several matches.", str);
            }
            match_found = 1;
            project->main_file = files.files[i];
        }
    }

    free_path(&path);
    if (!match_found) {
        log_fatal("No file matching \"%s\" could be found.", str);
    }
}

void add_build(bld_project* project, char* path) {
    project->build = path_from_string(path);
    ignore_path(project, path);
}

uintmax_t test_open(bld_path* path) {
    DIR* d;
    bld_dirent* file;
    int exists = 0;
    uintmax_t id;
    char* name;
    bld_path test = copy_path(path);
    
    name = remove_last_dir(&test);
    d = opendir(path_to_string(&test));

    while ((file = readdir(d)) != NULL) {
        if (strcmp(file->d_name, name) == 0) {
            exists = 1;
            id = file->d_ino;
            break;
        }
    }

    if (!exists) {
        log_fatal("\"%s\" is not a valid path under the root", path_to_string(path));
    }

    closedir(d);
    free_path(&test);
    return id;
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

void index_recursive(bld_project* project, bld_path* path) {
    int exists;
    char *str_path, *file_ending;
    bld_path sub_path;
    DIR* dir;
    bld_dirent* file_ptr;
    bld_file file;
    log_debug("Searching under: \"%s\"", path_to_string(path));

    str_path = path_to_string(path);
    dir = opendir(str_path);
    if (dir == NULL) {return;}

    while ((file_ptr = readdir(dir)) != NULL) {
        file_ending = file_ptr->d_name;
        if (strcmp(file_ptr->d_name, ".") == 0 || strcmp(file_ptr->d_name, "..") == 0) {
            continue;
        }
        
        for (size_t i = 0; i < project->ignore_paths.size; i++) {
            if (file_ptr->d_ino == project->ignore_paths.ids[i]) {
                log_debug("Ignoring: \"%s" BLD_PATH_SEP "%s\"", path_to_string(path), file_ptr->d_name); /* TODO: print better */
                goto next_file;
            }
        }

        if (file_ending[0] == '.') {continue;}

        sub_path = copy_path(path);
        append_dir(&sub_path, file_ptr->d_name);

        file_ending = strrchr(file_ending, '.');
        if (file_ending == NULL) {
            index_recursive(project, &sub_path);
            free_path(&sub_path);
            goto next_file;
        }

        if (strncmp(file_ptr->d_name, "test", 4) == 0 && strcmp(file_ending, ".c") == 0) {
            file = make_test(&sub_path, file_ptr);
        } else if (strcmp(file_ending, ".c") == 0) {
            file = make_impl(&sub_path, file_ptr);
        } else if (strcmp(file_ending, ".h") == 0) {
            file = make_header(&sub_path, file_ptr);
        } else {
            free_path(&sub_path);
            goto next_file;
        }

        exists = append_file(&project->files, file);
        if (exists) {
            free_file(&file);
            break;
        }

        next_file:;
    }
    
    closedir(dir);
}

void index_project(bld_project* project) {
    bld_path extra_path;
    DIR* dir;

    dir = opendir(path_to_string(&project->root));
    if (dir == NULL) {log_fatal("Could not open project root \"%s\"", path_to_string(&project->root));}
    closedir(dir);

    log_info("Indexing project under root");
    index_recursive(project, &project->root);

    for (size_t i = 0; i < project->extra_paths.size; i++) {
        extra_path = copy_path(&project->root);
        append_path(&extra_path, &project->extra_paths.paths[i]);
        log_info("Indexing files under \"%s\"", path_to_string(&extra_path));

        index_recursive(project, &extra_path);
        free_path(&extra_path);
    }

}

int compile_file(bld_project* project, bld_file* file) {
    int result;
    char name[256];
    bld_compiler compiler;
    bld_string cmd = new_string();
    bld_path path;

    if (file->compiler == NULL) {
        compiler = project->compiler;
    } else {
        compiler = *(file->compiler);
    }

    append_string(&cmd, compiler.executable);
    append_space(&cmd);

    for (size_t i = 0; i < compiler.options.size; i++) {
        append_string(&cmd, compiler.options.options[i]);
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

    result = system(make_string(&cmd));
    free_string(&cmd);
    return result;
}

int compile_total(bld_project* project, char* executable_name) {
    int result;
    char name[256];
    bld_path path;
    bld_compiler compiler = project->compiler;
    bld_file* file;
    bld_string cmd = new_string();
    bld_search_info* bfs;

    append_string(&cmd, compiler.executable);
    append_space(&cmd);

    for (size_t i = 0; i < compiler.options.size; i++) {
        append_string(&cmd, compiler.options.options[i]);
        append_space(&cmd);
    }

    append_string(&cmd, "-o ");
    path = copy_path(&project->root);
    append_dir(&path, executable_name);
    append_string(&cmd, path_to_string(&path));
    append_space(&cmd);
    free_path(&path);

    bfs = graph_dfs_from(&project->graph, &project->main_file);
    while (next_file(bfs, &file)) {
        path = copy_path(&project->root);
        append_path(&path, &(*project->cache).root);
        serialize_identifier(name, file);
        append_dir(&path, name);
        
        append_string(&cmd, path_to_string(&path));
        append_string(&cmd, ".o ");
        free_path(&path);
    }

    result = system(make_string(&cmd));

    free_string(&cmd);
    return result;
}

int compile_project(bld_project* project, char* name) {
    int result = 0, temp;
    uintmax_t hash;
    bld_path path;
    bld_files files = project->files;
    bld_file* file;

    hash = hash_compiler(&project->compiler, 5031);
    for (size_t i = 0; i < files.size; i++) {
        file = &files.files[i];
        if (file->type == BLD_HEADER) {continue;}

        file->identifier.hash = hash_file(file, hash);
        if (cached_compilation(project, file)) {
            continue;
        }

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

    generate_graph(&project->graph, &path);
    free_path(&path);

    temp = compile_total(project, name);
    if (temp) {
        log_warn("Could not compile final executable");
        result = temp;
    } else {
        log_info("Compiled executable: \"%s\"", name);
    }

    return result;
}

int cached_compilation(bld_project* project, bld_file* file) {
    log_warn("Searching cache for \"%s\", not found...", make_string(&file->name));
    return 0;
}

int test_project(bld_project* project, char* path) {
    log_warn("test_project: not implemented.");
    return -1;
}

bld_ignore new_ignore_ids() {
    return (bld_ignore) {
        .capacity = 0,
        .size = 0,
        .ids = NULL,
    };
}

void free_ignore_ids(bld_ignore* ignore) {
    free(ignore->ids);
}

void append_ignore_id(bld_ignore* ignore, uintmax_t id) {
    size_t capacity = ignore->capacity;
    uintmax_t* ids;

    if (ignore->size >= ignore->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        ids = malloc(capacity * sizeof(uintmax_t));
        if (ids == NULL) {log_fatal("Could not append id to ignore path");}

        memcpy(ids, ignore->ids, ignore->size * sizeof(uintmax_t));
        free(ignore->ids);

        ignore->capacity = capacity;
        ignore->ids = ids;
    }

    ignore->ids[ignore->size++] = id;
}

void print_project(bld_project* project) {
    /* TODO: make good :) */
    printf("Project:\n");
    
    printf("  root: \"%s\"\n", path_to_string(&project->root));
    
    printf("  extra_paths:");
    if (project->extra_paths.size == 0) {
        printf(" None\n");
    } else {
        printf("\n");
        for (size_t i = 0; i < project->extra_paths.size; i++) {
            printf("    \"%s\"\n", path_to_string(&project->extra_paths.paths[i]));
        }
    }

    printf("  compiler:\n");
    printf("    type: ");
    switch (project->compiler.type) {
        case (BLD_GCC): {
            printf("gcc\n");
        } break;
        case (BLD_CLANG): {
            printf("clang\n");
        } break;
        default: log_fatal("print_project: unknown compiler type.");
    }
    printf("    executable: \"%s\"\n", project->compiler.executable);
    printf("    options:");
    if (project->compiler.options.size == 0) {
        printf(" None\n");
    } else {
        printf("\n");
        for (size_t i = 0; i < project->compiler.options.size; i++) {
            printf("      \"%s\"\n", project->compiler.options.options[i]);
        }
    }

    printf("  files:");
    if (project->files.size == 0) {
        printf(" None\n");
    } else {
        printf("\n");
        for (size_t i = 0; i < project->files.size; i++) {
            printf("    File: \"%s\"\n", make_string(&project->files.files[i].name));
            printf("      type: ");
            switch (project->files.files[i].type) {
                case (BLD_IMPL): {
                    printf("implementation");
                } break;
                case (BLD_HEADER): {
                    printf("header");
                } break;
                case (BLD_TEST): {
                    printf("test");
                } break;
                default: log_fatal("print_project: unknown file_type");
            }
            printf("\n");
            printf("      path: \"%s\"\n", path_to_string(&project->files.files[i].path));
            if (project->files.files[i].compiler != NULL) {
                printf("      compiler:\n");
                printf("        type: ");
                switch (project->files.files[i].compiler->type) {
                    case (BLD_GCC): {
                        printf("gcc\n");
                    } break;
                    case (BLD_CLANG): {
                        printf("clang\n");
                    } break;
                    default: log_fatal("print_project: unknown compiler type.");
                }
                printf("        executable: \"%s\"\n", project->files.files[i].compiler->executable);
                printf("        options:");
                if (project->files.files[i].compiler->options.size == 0) {
                    printf(" None\n");
                } else {
                    printf("\n");
                    for (size_t i = 0; i < project->files.files[i].compiler->options.size; i++) {
                        printf("          \"%s\"\n", project->files.files[i].compiler->options.options[i]);
                    }
                }
            }
        }
    }

}
