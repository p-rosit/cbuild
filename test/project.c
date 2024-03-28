#include "../cbuild.h"

int main(int argc, char** argv) {
    int result = 0;
    bld_compiler project_compiler;
    bld_forward_project forward_project;
    bld_project project;

    project_compiler = compiler_new(BLD_GCC, "/usr/bin/gcc");
    compiler_add_flag(&project_compiler, "-lm");
    compiler_add_flag(&project_compiler, "-fsanitize=address");
    compiler_add_flag(&project_compiler, "-std=c99");
    compiler_add_flag(&project_compiler, "-Wall");
    compiler_add_flag(&project_compiler, "-Wextra");
    compiler_add_flag(&project_compiler, "-pedantic");

    forward_project = project_new(project_path_extract(argc, argv), project_compiler);

    project_add_build(&forward_project, "..");
    rebuild_builder(&forward_project, argc, argv);

    /* Optional */
    project_load_cache(&forward_project, ".build_cache");

    /* Optional, add specific compiler to any files in project */
    project_set_compiler(
        &forward_project, "file.c",
        compiler_with_flags(
            BLD_CLANG, "/usr/bin/clang",
            "-Weverything", NULL
        )
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

    /* Optional */
    project_save_cache(&project);

    /* Mandatory */
    project_free(&project);










    /* Mandatory */
    incremental_index_project(&project);

    /* Optional, mandatory if main file set */
    // generate_dependency_graph(&project);

    /* Optional */
    /* Mandatory */
    result = incremental_compile_project(&project, "a.out");
    if (result > 0) {
        log_warn("Could not compile project");
    } else if (result < 0) {
        log_info("Entire project existed in cache");
    }

    project_save_cache(&project);

    project_free(&project);
    return 0;
}
