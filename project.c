#include "build.h"
#include "project.h"



bld_path extract_root(int argc, char** argv) {
    /* TODO: argv[0] is not guaranteed to contain path to executable */
    bld_path path = new_path();
    append_dir(&path, argv[0]);
    remove_last_dir(&path);
    log_info("Extracted path to root: \"%.*s\"", (int) path.str.size, path.str.chars);
    return path;
}

bld_project new_project(bld_path path, bld_compiler compiler) {
    bld_files* f = malloc(sizeof(bld_files));
    bld_compiler* c = malloc(sizeof(bld_compiler));

    if (f == NULL) {log_fatal("Could not allocate files to project.");}
    *f = new_files();
    if (c == NULL) {log_fatal("Could not allocate compiler to project.");}
    *c = compiler;

    return (bld_project) {
        .root = path,
        .compiler = c,
        .files = f,
        .cache = NULL,
    };
}

void free_project(bld_project project) {
    free_path(&project.root);

    free_compiler(project.compiler);
    free(project.compiler);
    
    free_files(project.files);
    free(project.files);
    
    free_cache(project.cache);
    free(project.cache);
}

void load_cache(bld_project project, char* cache_path) {
    log_warn("load_cache: not implemented.");
}

void save_cache(bld_project project) {
    log_warn("save_cache: not implemented.");
}

void index_project(bld_project project) {
    log_warn("index_project: not implemented.");
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
