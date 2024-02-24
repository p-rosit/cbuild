#include "build.h"
#include "rebuild.h"

void rebuild_builder(bld_project* project, int argc, char** argv) {
    bld_path build_root;
    bld_compiler compiler;
    bld_project build;

    {
        log_warn("Rebuilding not implemented.");
        return;
    }

    build_root = copy_path(&project->root);
    append_path(&build_root, &project->build);

    compiler = copy_compiler(&project->compiler);
    build = new_project(build_root, compiler);

    add_option(&build.compiler, "-std=c99");
    add_option(&build.compiler, "-Wall");
    add_option(&build.compiler, "-Wextra");
    add_option(&build.compiler, "-Werror");
    add_option(&build.compiler, "-pedantic");

    load_cache(&build, ".build_cache");
    index_project(&build);

    // set_main_file(project, <same main as current>);
    // compile_project(build, <same name as current>)
    // Detect if any recompilation happened...
    // Run new build script if recompilation succeded

    save_cache(&build);

    free_project(&build);
}
