#include "bld_core/cbuild.h"

int main(int argc, char** argv) {
    int result = 0;
    bld_forward_project fproject;
    bld_project project;

    fproject = project_new(
        project_path_extract(argc, argv),
        compiler_with_flags(
            BLD_COMPILER_CLANG,
            "clang",
            "-std=c89",
            "-fsanitize=address",
            "-g",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
            "-Wmissing-prototypes",
            NULL
        ),
        linker_with_flags(
            "clang",
            "-fsanitize=address",
            NULL
        )
    );

    project_add_path(&fproject, "bld_core");
    project_ignore_path(&fproject, "bld_core/test");
    project_add_build(&fproject, "bld_core");
    rebuild_builder(&fproject, argc, argv);

    project_load_cache(&fproject, ".build_cache");

    project_set_main_file(&fproject, "main.c");

    project = project_resolve(&fproject);

    result = incremental_compile_project(&project, "bld.out");
    if (result > 0) {
        log_warn("Could not compile project");
    } else if (result < 0) {
        log_info("Entire project existed in cache");
    }

    project_save_cache(&project);

    project_free(&project);
    return 0;
}
