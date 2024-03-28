#include "../cbuild.h"

int main(int argc, char** argv) {
    int result = 0;
    bld_project project = project_new(
        project_path_extract(argc, argv),
        compiler_new(BLD_GCC, "/usr/bin/gcc")
    );

    project_add_build(&project, "..");
    rebuild_builder(&project, argc, argv);

    compiler_add_flag(&project.compiler, "-lm");
    compiler_add_flag(&project.compiler, "-fsanitize=address");
    compiler_add_flag(&project.compiler, "-std=c99");
    compiler_add_flag(&project.compiler, "-Wall");
    compiler_add_flag(&project.compiler, "-Wextra");
    compiler_add_flag(&project.compiler, "-pedantic");

    bld_compiler cc = compiler_new(BLD_CLANG, "/usr/bin/clang");
    compiler_add_flag(&cc, "-Weverything");
    {

        /* Optional */
        project_load_cache(&project, ".build_cache");

        /* Mandatory */
        incremental_index_project(&project);

        /* Optional, mandatory if main file set */
        // generate_dependency_graph(&project);

        /* Optional */
        project_set_compiler(&project, "file.c", cc);

        /* Optional */
        project_set_main_file(&project, "main.c");
        /* Mandatory */
        result = incremental_compile_project(&project, "a.out");
        if (result > 0) {
            log_warn("Could not compile project");
        } else if (result < 0) {
            log_info("Entire project existed in cache");
        }

        project_save_cache(&project);
    }

    project_free(&project);
    return 0;
}
