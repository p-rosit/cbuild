#include <string.h>
#include "build.h"
#include "file.h"
#include "project.h"



bld_path extract_root(int argc, char** argv) {
    /* TODO: argv[0] is not guaranteed to contain path to executable */
    bld_path path = path_from_string(argv[0]);
    remove_last_dir(&path);
    log_info("Extracted path to root: \"%.*s\"", (int) path.str.size, path.str.chars);
    return path;
}

bld_project new_project(bld_path path, bld_compiler compiler) {
    bld_files* f = malloc(sizeof(bld_files));
    bld_compiler* c = malloc(sizeof(bld_compiler));
    bld_extra* e = malloc(sizeof(bld_extra));
    bld_cache** cache = malloc(sizeof(bld_cache*));

    if (f == NULL) {log_fatal("Could not allocate files to project.");}
    *f = new_files();
    if (c == NULL) {log_fatal("Could not allocate compiler to project.");}
    *c = compiler;
    if (e == NULL) {log_fatal("Could not allocate extra paths to project.");}
    *e = (bld_extra) {.capacity = 0, .size = 0, .paths = NULL};
    if (cache == NULL) {log_fatal("Could not allocate cache.");}
    *cache = NULL;

    return (bld_project) {
        .root = path,
        .extra_paths = e,
        .compiler = c,
        .files = f,
        .cache = cache,
    };
}

void free_project(bld_project project) {
    free_path(&project.root);

    for (size_t i = 0; i < project.extra_paths->size; i++) {
        free_path(&project.extra_paths->paths[i]);
    }
    free(project.extra_paths->paths);
    free(project.extra_paths);

    free_compiler(project.compiler);
    free(project.compiler);
    
    free_files(project.files);
    free(project.files);
    
    free_cache(*(project.cache));
    free(*(project.cache));
    free(project.cache);
}

void append_extra_path(bld_extra* extra, char* path) {
    size_t capacity = extra->capacity;
    bld_path* paths;

    if (extra->size >= extra->capacity) {
        capacity += (capacity / 2) + 2 * (capacity < 2);

        paths = malloc(capacity * sizeof(bld_path));
        if (paths == NULL) {log_fatal("Could not add path \"%s\"", path);}

        memcpy(paths, extra->paths, extra->size * sizeof(bld_path));
        free(extra->paths);

        extra->paths = paths;
        extra->capacity = capacity;
    }

    extra->paths[extra->size++] = path_from_string(path);
}

void add_path(bld_project project, char* path) {
    append_extra_path(project.extra_paths, path);
}

void index_recursive(bld_project project, bld_path* path) {
    int exists;
    char *str_path, *file_ending;
    bld_path sub_path;
    DIR* dir;
    bld_dirent* file_ptr;
    bld_file file;

    str_path = path_to_string(path);
    dir = opendir(str_path);
    if (dir == NULL) {return;}

    while ((file_ptr = readdir(dir)) != NULL) {
        file_ending = file_ptr->d_name;
        if (file_ending[0] == '.') {continue;}

        sub_path = copy_path(path);
        append_dir(&sub_path, file_ptr->d_name);

        file_ending = strrchr(file_ending, '.');
        if (file_ending == NULL) {
            index_recursive(project, &sub_path);
            free_path(&sub_path);
            continue;
        }

        if (strncmp(file_ptr->d_name, "test", 4) == 0 && strcmp(file_ending, ".c") == 0) {
            file = make_test(&sub_path, file_ptr);
        } else if (strcmp(file_ending, ".c") == 0) {
            file = make_impl(&sub_path, file_ptr);
        } else if (strcmp(file_ending, ".h") == 0) {
            file = make_header(&sub_path, file_ptr);
        } else {
            free_path(&sub_path);
            continue;
        }

        exists = append_file(project.files, file);
        if (exists) {
            free_file(&file);
            break;
        }
    }
    
    closedir(dir);
}

void index_project(bld_project project) {
    bld_path extra_path;
    DIR* dir;

    dir = opendir(path_to_string(&project.root));
    if (dir == NULL) {log_fatal("Could not open project root \"%s\"", path_to_string(&project.root));}
    closedir(dir);

    log_info("Indexing project under root");
    index_recursive(project, &project.root);

    for (size_t i = 0; i < project.extra_paths->size; i++) {
        extra_path = copy_path(&project.root);
        append_path(&extra_path, &project.extra_paths->paths[i]);
        log_info("Indexing project under \"%s\"", path_to_string(&extra_path));
        index_recursive(project, &extra_path);
        free_path(&extra_path);
    }
}

int compile_project(bld_project project) {
    log_warn("compile_project: not implemented.");
    return -1;
}

int make_executable(bld_project project, char* name) {
    log_warn("make_executable: not implemented");
    return -1;
}

int test_project(bld_project project, char* path) {
    log_warn("test_project: not implemented.");
    return -1;
}

void print_project(bld_project project) {
    /* TODO: make good :) */
    printf("Project:\n");
    
    printf("  root: \"%s\"\n", path_to_string(&project.root));
    
    printf("  extra_paths:");
    if (project.extra_paths->size == 0) {
        printf(" None\n");
    } else {
        printf("\n");
        for (size_t i = 0; i < project.extra_paths->size; i++) {
            printf("    \"%s\"\n", path_to_string(&project.extra_paths->paths[i]));
        }
    }

    printf("  compiler:\n");
    printf("    type: ");
    switch (project.compiler->type) {
        case (BLD_GCC): {
            printf("gcc\n");
        } break;
        case (BLD_CLANG): {
            printf("clang\n");
        } break;
        default: log_fatal("print_project: unknown compiler type.");
    }
    printf("    executable: \"%s\"\n", project.compiler->executable);
    printf("    options:");
    if (project.compiler->options.size == 0) {
        printf(" None\n");
    } else {
        printf("\n");
        for (size_t i = 0; i < project.compiler->options.size; i++) {
            printf("      \"%s\"\n", project.compiler->options.options[i]);
        }
    }

    printf("  files:");
    if (project.files->size == 0) {
        printf(" None\n");
    } else {
        printf("\n");
        for (size_t i = 0; i < project.files->size; i++) {
            printf("    File: \"%s\"\n", make_string(&project.files->files[i].name));
            printf("      type: ");
            switch (project.files->files[i].type) {
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
            printf("      path: \"%s\"\n", path_to_string(&project.files->files[i].path));
            if (project.files->files[i].compiler != NULL) {
                printf("      compiler:\n");
                printf("        type: ");
                switch (project.files->files[i].compiler->type) {
                    case (BLD_GCC): {
                        printf("gcc\n");
                    } break;
                    case (BLD_CLANG): {
                        printf("clang\n");
                    } break;
                    default: log_fatal("print_project: unknown compiler type.");
                }
                printf("        executable: \"%s\"\n", project.files->files[i].compiler->executable);
                printf("        options:");
                if (project.files->files[i].compiler->options.size == 0) {
                    printf(" None\n");
                } else {
                    printf("\n");
                    for (size_t i = 0; i < project.files->files[i].compiler->options.size; i++) {
                        printf("          \"%s\"\n", project.files->files[i].compiler->options.options[i]);
                    }
                }
            }
        }
    }

}
