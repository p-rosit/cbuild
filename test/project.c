#include "../cbuild.h"

int main(int argc, char** argv) {
    bld_project project = new_project(
        extract_path(argc, argv),
        new_compiler(BLD_GCC, "/usr/bin/gcc")
    );

    add_build(&project, "..");
    rebuild_builder(&project, argc, argv);

    add_option(&project.compiler, "-lm");
    add_option(&project.compiler, "-fsanitize=address");
    // add_option(project.compiler, "-Wall");
    // add_option(project.compiler, "-Wextra");
    // add_option(project.compiler, "-pedantic");

    bld_compiler cc = new_compiler(BLD_CLANG, "/usr/bin/clang");
    {
        add_option(&cc, "-LL");

        load_cache(&project, ".build_cache");

        index_project(&project);
        set_main_file(&project, "main.c");
        
        project.files.files[1].compiler = &cc;

        compile_project(&project, "a.out");

        save_cache(&project);
    }

    free_project(&project);
    return 0;
}
