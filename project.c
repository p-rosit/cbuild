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

    if (f == NULL) {log_fatal("Could not allocate files to project.");}
    *f = new_files();
    if (c == NULL) {log_fatal("Could not allocate compiler to project.");}
    *c = compiler;
    if (e == NULL) {log_fatal("Could not allocate extra paths to project.");}
    *e = (bld_extra) {.capacity = 0, .size = 0, .paths = NULL};

    return (bld_project) {
        .root = path,
        .extra_paths = e,
        .compiler = c,
        .files = f,
        .cache = NULL,
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
    
    free_cache(project.cache);
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

void load_cache(bld_project project, char* cache_path) {
    log_warn("load_cache: not implemented.");
}

void save_cache(bld_project project) {
    log_warn("save_cache: not implemented.");
}

void index_recursive(bld_project project, bld_path* path) {
    char *str_path, *file_ending;
    bld_path sub_path;
    DIR* dir;
    bld_dirent* file_ptr;
    bld_file file;

    log_info("Indexing at: \"%.*s\"", (int) path->str.size, path->str.chars);

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

        append_file(project.files, file);
    }
    
    closedir(dir);
}

void index_project(bld_project project) {
    char* str;
    DIR* dir;

    str = path_to_string(&project.root);
    dir = opendir(str);
    if (dir == NULL) {log_fatal("Could not open project root \"%s\"", str);}
    closedir(dir);

    index_recursive(project, &project.root);
    log_info("Indexed project");
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
