#include "../cbuild.h"

int main(int argc, char** argv) {
    REBUILD_BUILDER(argc, argv);

    bld_path root       = extract_root(argc, argv);
    bld_compiler gcc    = new_compiler(BLD_GCC, "/usr/bin/gcc");
    bld_project project = new_project(root, gcc);

    add_path(project, "..");

    add_option(project.compiler, "-fsanitize=address");
    add_option(project.compiler, "-Wall");
    add_option(project.compiler, "-Wextra");
    add_option(project.compiler, "-pedantic");

    {
        load_cache(project, ".build_cache");

        index_project(project);
        compile_project(project);
        make_executable(project, "a.out");

        save_cache(project);
    }

    free_project(project);
    return 0;
}
