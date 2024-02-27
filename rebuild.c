#include <string.h>
#include "build.h"
#include "rebuild.h"

bld_project make_project(bld_path, bld_compiler);
bld_project new_rebuild(bld_path root, bld_compiler compiler) {
    bld_project build;
    build = make_project(root, compiler);
    return build;
}

void extract_names(int argc, char** argv, char** file, char** old_file) {
    bld_path path = extract_path(argc, argv);
    bld_string str;
    char* name;
    
    name = remove_last_dir(&path);
    str = new_string();
    append_string(&str, name);
    *file = make_string(&str);

    str = new_string();
    append_string(&str, "old_");
    append_string(&str, name);
    *old_file = make_string(&str);

    free_path(&path);
}

char* infer_build_name(char* name) {
    bld_path path;

    path = path_from_string(name);
    remove_file_ending(&path);
    append_string(&path.str, ".c");

    log_debug("Build name: \"%s\"", path_to_string(&path));
    return path_to_string(&path);
}

void index_possible_file(bld_project*, bld_path*, char*);
void rebuild_builder(bld_project* project, int argc, char** argv) {
    int result;
    char *executable, *old_executable, *main_name;
    bld_path build_root, main;
    bld_compiler compiler = new_compiler(BLD_GCC, "/usr/bin/gcc"); /* TODO: don't hardcode compiler */
    bld_project build;

    extract_names(argc, argv, &executable, &old_executable);
    main_name = infer_build_name(executable);

    log_debug("Exe:  \"%s\"", executable);
    log_debug("Old:  \"%s\"", old_executable);
    log_debug("Main: \"%s\"", main_name);

    build_root = copy_path(&project->root);
    append_path(&build_root, &project->build);
    log_debug("Root: \"%s\"", path_to_string(&build_root));

    build = new_rebuild(build_root, compiler);
    ignore_path(&build, "./test");

    add_option(&build.compiler, "-std=c99");
    add_option(&build.compiler, "-Wall");
    add_option(&build.compiler, "-Wextra");
    add_option(&build.compiler, "-Werror");
    add_option(&build.compiler, "-pedantic");

    load_cache(&build, ".build_cache");

    main = copy_path(&project->root);
    append_dir(&main, main_name);
    index_possible_file(&build, &main, main_name);
    index_project(&build);

    build.main_file = build.files.files[0];

    result = compile_project(&build, "./test/b.out");
    if (result > 0) {
        log_warn("Could not compile build script");
    } else if (result < 0) {
        log_warn("No rebuild neccessary");
    } else {
        log_warn("Recompiled build script");
    }

    save_cache(&build);

    free(executable);
    free(old_executable);
    free(main_name);
    free_path(&main);
    free_project(&build);
    log_fatal("rebuild: not implemented");
}
