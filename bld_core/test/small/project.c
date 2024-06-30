#include "../../cbuild.h"

int main(int argc, char** argv) {
    int result = 0;
    bld_forward_project forward_project;
    bld_project project;
    bld_compiler compiler;

    compiler = compiler_new(BLD_COMPILER_GCC, "gcc");
    compiler_add_flag(&compiler, "-std=c99");
    compiler_add_flag(&compiler, "-Wall");
    compiler_add_flag(&compiler, "-Wextra");
    compiler_add_flag(&compiler, "-pedantic");

    forward_project = project_new(project_path_extract(argc, argv), compiler, linker_new(BLD_LINKER_GCC, "gcc"));

    project_add_build(&forward_project, "../..");
    rebuild_builder(&forward_project, argc, argv);

    /* Optional */
    project_load_cache(&forward_project, ".build_cache");

    /* Optional, add specific compiler to any files in project */
    {
        bld_compiler compiler;
        compiler = compiler_new(BLD_COMPILER_CLANG, "clang");
        compiler_add_flag(&compiler, "-Weverything");

        project_set_compiler(&forward_project, "file.c", compiler);
    }
    {
        bld_linker_flags linker_flags;
        linker_flags = linker_flags_new();
        linker_flags_add_flag(&linker_flags, "-lm");

        project_set_linker_flags(&forward_project, "dist.c", linker_flags);
    }

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
