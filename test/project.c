#include "../cbuild.h"

int main(int argc, char** argv) {
    int result = 0;
    bld_forward_project forward_project;
    bld_project project;

    forward_project = project_new(
        project_path_extract(argc, argv),
        compiler_with_flags(
            "/usr/bin/gcc",
            "-lm",
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-pedantic",
            NULL
        ),
        linker_new("/usr/bin/gcc")
    );

    project_add_build(&forward_project, "..");
    rebuild_builder(&forward_project, argc, argv);

    /* Optional */
    project_load_cache(&forward_project, ".build_cache");

    /* Optional, add specific compiler to any files in project */
    project_set_compiler(
        &forward_project, "file.c",
        compiler_with_flags("/usr/bin/clang", "-Weverything", NULL)
    );
    project_set_linker_flags(
        &forward_project, "file.c",
        linker_flags_with_flags("-lm", NULL)
    );

    /* Mandatory */
    project_set_main_file(&forward_project, "main.c");

    /* Mandatory */
    project = project_resolve(&forward_project);

    /* Mandatory */
    result = incremental_compile_project(&project, "a.out");
    if (result > 0) {
        log_warn("Could not compile project");
    } else if (result < 0) {
        log_info("Entire project existed in cache");
    }

    /* Optional, can only be used if cache path has been set with project_load_cache */
    project_save_cache(&project);

    /* Mandatory */
    project_free(&project);
    return 0;
}
