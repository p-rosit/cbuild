#include <stdio.h>
#include <string.h>
#include "logging.h"
#include "incremental.h"
#include "rebuild.h"

int                 run_new_build(bld_path*, char*);
bld_forward_project new_rebuild(bld_forward_project*, bld_path, bld_compiler, bld_linker);
void                extract_names(int, char**, char**, char**);
char*               infer_build_name(char*);
void                set_main_rebuild(bld_forward_project*, bld_path*);

int run_new_build(bld_path* root, char* executable) {
    int result;
    bld_path cmd;

    cmd = path_copy(root);
    path_append_string(&cmd, executable);

    log_info("Running new build script");
    log_debug("Rebuild command: \"%s\"", path_to_string(&cmd));
    result = system(path_to_string(&cmd));

    path_free(&cmd);
    return result;
}

bld_forward_project new_rebuild(bld_forward_project* fproject, bld_path root, bld_compiler compiler, bld_linker linker) {
    bld_forward_project fbuild;
    fbuild = project_forward_new(&root, &compiler, &linker);
    fbuild.base.rebuilding = 1;
    fbuild.base.build_of = &fproject->base;
    return fbuild;
}

void extract_names(int argc, char** argv, char** file, char** old_file) {
    bld_path path;
    bld_string str;
    char* name;
    
    path = project_path_extract(argc, argv);
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

void set_main_rebuild(bld_forward_project* build, bld_path* path) {
    bld_string str;
    str = string_new();
    string_append_string(&str, path_to_string(path));
    build->main_file_name = str;
}

void    project_base_free(bld_project_base*);
int     incremental_compile_with_absolute_path(bld_project*, char*);
void rebuild_builder(bld_forward_project* fproject, int argc, char** argv) {
    int result, new_result, log_level;
    char *executable, *old_executable, *main_name;
    bld_path build_root, main, executable_path;
    bld_forward_project fbuild;
    bld_project build;
    bld_compiler compiler;
    bld_linker linker;

    if (fproject->base.standalone) {
        log_fatal("rebuild_builder: attempting to rebuild build script but not build path has been set");
        return;  /* unreachable */
    }

    log_level = set_log_level(BLD_WARN);

    extract_names(argc, argv, &executable, &old_executable);
    main_name = infer_build_name(executable);

    log_debug("Exe:  \"%s\"", executable);
    log_debug("Old:  \"%s\"", old_executable);
    log_debug("Main: \"%s\"", main_name);

    /* TODO: verify that build path has been set */
    build_root = path_copy(&fproject->base.root);
    path_append_path(&build_root, &fproject->base.build);
    log_debug("Root: \"%s\"", path_to_string(&build_root));

    compiler = compiler_new(BLD_COMPILER_GCC, "gcc");
    compiler_add_flag(&compiler, "-std=c89");
    compiler_add_flag(&compiler, "-fsanitize=address");
    compiler_add_flag(&compiler, "-g");
    compiler_add_flag(&compiler, "-Wall");
    compiler_add_flag(&compiler, "-Wextra");
    compiler_add_flag(&compiler, "-Werror");
    compiler_add_flag(&compiler, "-pedantic");
    compiler_add_flag(&compiler, "-Wmissing-prototypes");

    linker = linker_new(BLD_LINKER_GCC, "gcc");
    linker_add_flag(&linker, "-fsanitize=address");

    fbuild = new_rebuild(fproject, build_root, compiler, linker);
    project_ignore_path(&fbuild, "./test");

    main = path_copy(&fproject->base.root);
    path_append_string(&main, main_name);
    set_main_rebuild(&fbuild, &main);

    project_load_cache(&fbuild, ".build_cache");

    build = project_resolve(&fbuild);

    executable_path = path_copy(&fproject->base.root);
    path_append_string(&executable_path, executable);

    {
        bld_path temp1, temp2;

        temp1 = path_copy(&fproject->base.root);
        temp2 = path_copy(&fproject->base.root);

        path_append_string(&temp1, executable);
        path_append_string(&temp2, old_executable);

        log_debug("Renaming old executable: %s -> %s", path_to_string(&temp1), path_to_string(&temp2));
        rename(path_to_string(&temp1), path_to_string(&temp2));

        path_free(&temp1);
        path_free(&temp2);
    }

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
        new_result = run_new_build(&fproject->base.root, executable);
    } else {
        new_result = -1;
    }

    free(executable);
    free(old_executable);
    free(main_name);
    path_free(&executable_path);
    path_free(&main);
    project_free(&build);

    if (result == 0) {
        project_base_free(&fproject->base);
        project_partial_free(fproject);
        exit(new_result);
    }

    set_log_level(log_level);
}
