#include "../cbuild.h"

/**
 *  TODO: find include dependencies and put in graph
 *  TODO: make graph optional
 *  TODO: make cache optional
 *  TODO: recompile if file changed due to include dependency
 *  TODO: use cache to populate graph
 *  TODO: split generating dependency graph from indexing project
 *  TODO: detect when .c file was included and disregard when compiling
 *  TODO: detect if .h file is header only
 *  TODO: use hash maps where appropriate
 */

int main(int argc, char** argv) {
    int result = 0;
    bld_project project = new_project(
        extract_path(argc, argv),
        new_compiler(BLD_GCC, "/usr/bin/gcc")
    );

    add_build(&project, "..");
    rebuild_builder(&project, argc, argv);

    add_option(&project.compiler, "-lm");
    add_option(&project.compiler, "-fsanitize=address");
    add_option(&project.compiler, "-std=c99");
    add_option(&project.compiler, "-Wall");
    add_option(&project.compiler, "-Wextra");
    add_option(&project.compiler, "-pedantic");

    bld_compiler cc = new_compiler(BLD_CLANG, "/usr/bin/clang");
    add_option(&cc, "-Weverything");
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
