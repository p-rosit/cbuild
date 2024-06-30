#include "bld_core/cbuild.h"

int main(int argc, char** argv) {
    int result;
    bld_forward_project fproject;
    bld_project project;
    bld_compiler compiler;
    bld_linker linker;

    compiler = compiler_new(BLD_COMPILER_CLANG, "clang");
    compiler_add_flag(&compiler, "-std=c89");
    compiler_add_flag(&compiler, "-fsanitize=address");
    compiler_add_flag(&compiler, "-g");
    compiler_add_flag(&compiler, "-Wall");
    compiler_add_flag(&compiler, "-Wextra");
    compiler_add_flag(&compiler, "-Werror");
    compiler_add_flag(&compiler, "-pedantic");
    compiler_add_flag(&compiler, "-Wmissing-prototypes");

    linker = linker_new(BLD_LINKER_CLANG, "clang");
    linker_add_flag(&linker, "-fsanitize=address");

    fproject = project_new(project_path_extract(argc, argv), compiler, linker);

    project_add_build(&fproject, "bld_core");
    project_ignore_path(&fproject, "bld_core/test");
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
