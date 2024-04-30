#include "../../cbuild.h"


int main(int argc, char** argv) {
    bit_forward_project fproject;
    bit_project project;

    fproject = project_new(
        project_path_extract(argc, argv),
        compiler_new("gcc"),
        linker_with_flags("gcc", "-lstdc++", NULL)
    );

    project_add_build(&fproject, "../..");
    rebuild_builder(&fproject, argc, argv);

    project_load_cache(&fproject, ".build_cache");

    project_set_main_file(&fproject, "main.cpp");
    project = project_resolve(&fproject);

    incremental_compile_project(&project, "a.out");

    project_save_cache(&project);

    project_free(&project);
    return 0;
}
