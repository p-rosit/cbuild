#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "incremental.h"
#include "rebuild.h"

int         run_new_build(bld_path*, char*);
bld_project new_rebuild(bld_path, bld_compiler);
void        extract_names(int, char**, char**, char**);
char*       infer_build_name(char*);
void        set_main_rebuild(bld_project*, bld_path*);

int run_new_build(bld_path* root, char* executable) {
    int result;
    bld_path cmd = path_copy(root);
    path_append_string(&cmd, executable);

    log_info("Running new build script");
    log_debug("Rebuild command: \"%s\"", path_to_string(&cmd));
    result = system(path_to_string(&cmd));

    path_free(&cmd);
    return result;
}

bld_project make_project(bld_path, bld_compiler);
bld_project new_rebuild(bld_path root, bld_compiler compiler) {
    bld_project build;
    build = make_project(root, compiler);
    return build;
}

void extract_names(int argc, char** argv, char** file, char** old_file) {
    bld_path path = project_path_extract(argc, argv);
    bld_string str;
    char* name;
    
    name = path_remove_last_string(&path);
    if (strncmp(name, "old_", 4) == 0) {
        name += 4;
    }

    str = string_new();
    string_append_string(&str, name);
    *file = string_unpack(&str);

    str = string_new();
    string_append_string(&str, "old_");
    string_append_string(&str, name);
    *old_file = string_unpack(&str);

    path_free(&path);
}

char* infer_build_name(char* name) {
    bld_path path;

    path = path_from_string(name);
    path_remove_file_ending(&path);
    string_append_string(&path.str, ".c");

    return path_to_string(&path);
}

void set_main_rebuild(bld_project* build, bld_path* path) {
    struct stat file_stat;
    bld_file* file;

    if (stat(path_to_string(path), &file_stat) < 0) {
        log_fatal("Expected \"%s\" to be a valid path to the build file", path_to_string(path));
    }

    file = set_get(&build->files, file_stat.st_ino);
    
    if (file == NULL) {log_fatal("Main file has not been indexed");}

    build->main_file = file->identifier.id;
}

void incremental_index_possible_file(bld_project*, bld_path*, char*);
int incremental_compile_with_absolute_path(bld_project*, char*);
void rebuild_builder(bld_project* project, int argc, char** argv) {
    int result, new_result, log_level;
    char *executable, *old_executable, *main_name;
    bld_path build_root, main, executable_path;
    bld_compiler compiler = compiler_new(BLD_GCC, "/usr/bin/gcc"); /* TODO: don't hardcode compiler */
    bld_project build;

    log_level = set_log_level(BLD_WARN);

    extract_names(argc, argv, &executable, &old_executable);
    main_name = infer_build_name(executable);

    log_debug("Exe:  \"%s\"", executable);
    log_debug("Old:  \"%s\"", old_executable);
    log_debug("Main: \"%s\"", main_name);

    build_root = path_copy(&project->root);
    path_append_path(&build_root, &project->build);
    log_debug("Root: \"%s\"", path_to_string(&build_root));

    build = new_rebuild(build_root, compiler);
    project_ignore_path(&build, "./test");

    compiler_add_flag(&build.compiler, "-std=c99");
    compiler_add_flag(&build.compiler, "-fsanitize=address");
    compiler_add_flag(&build.compiler, "-Wall");
    compiler_add_flag(&build.compiler, "-Wextra");
    compiler_add_flag(&build.compiler, "-Werror");
    compiler_add_flag(&build.compiler, "-pedantic");
    compiler_add_flag(&build.compiler, "-Wmissing-prototypes");

    project_load_cache(&build, ".build_cache");

    main = path_copy(&project->root);
    path_append_string(&main, main_name);
    incremental_index_possible_file(&build, &main, main_name);
    incremental_index_project(&build);

    set_main_rebuild(&build, &main);

    executable_path = path_copy(&project->root);
    path_append_string(&executable_path, executable);
    rename(executable, old_executable);
    result = incremental_compile_with_absolute_path(&build, path_to_string(&executable_path));

    if (result > 0) {
        log_fatal("Could not compile build script");
    } else if (result < 0) {
        log_debug("No rebuild neccessary");
    }

    project_save_cache(&build);

    if (result == 0) {
        set_log_level(BLD_INFO);
        log_info("Recompiled build script");
        new_result = run_new_build(&project->root, executable);
    }

    free(executable);
    free(old_executable);
    free(main_name);
    path_free(&executable_path);
    path_free(&main);
    project_free(&build);

    if (result == 0) {
        project_free(project);
        exit(new_result);
    }

    set_log_level(log_level);
}
