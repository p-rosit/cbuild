#include "../cbuild.h"

int main(int argc, char** argv) {
    int result = 0;
    bld_project project = new_project(
        extract_path(argc, argv),
        compiler_new(BLD_GCC, "/usr/bin/gcc")
    );

    add_build(&project, "..");
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
        load_cache(&project, ".build_cache");

        /* Mandatory */
        index_project(&project); 

        /* Optional, mandatory if main file set */
        // generate_dependency_graph(&project);

        /* Optional */
        set_compiler(&project, "file.c", cc);

        /* Optional */
        set_main_file(&project, "main.c");
        /* Mandatory */
        result = compile_project(&project, "a.out");
        if (result > 0) {
            log_warn("Could not compile project");
        } else if (result < 0) {
            log_info("Entire project existed in cache");
        }

        save_cache(&project);
    }

    free_project(&project);
    return 0;
}
