#include "logging.h"
#include "project_testing.h"
#include "incremental.h"

void project_tests_under_recursive(bld_array*, bld_file*, bld_set*);

int project_test_files(bld_project* project, bld_array* files) {
    int error;
    size_t total_succeded;
    bld_iter iter;
    bld_file* file;

    error = 0;
    total_succeded = 0;
    iter = iter_array(files);
    while (iter_next(&iter, (void**) &file)) {
        int temp;

        temp = project_test_file(project, file);
        if (temp) {
            error = temp;
        } else {
            total_succeded += 1;
        }
    }

    printf("Test result: %lu/%lu\n", total_succeded, files->size);
    return error;
}

int project_test_file(bld_project* project, bld_file* file) {
    int error;
    bld_path test_path;
    bld_string test_name;

    test_name = file_object_name(file);
    test_path = path_from_string(".");
    path_append_string(&test_path, string_unpack(&test_name));

    project->main_file = file->identifier.id;
    error = incremental_compile_executable(project, path_to_string(&test_path));
    if (error) {
        log_warn("Test: '%s', could not compile project", path_to_string(&file->path));
    }

    error = system(path_to_string(&test_path));
    remove(path_to_string(&test_path));

    if (!error) {
        printf("Test '%s' ok\n", path_to_string(&file->path));
    } else {
        printf("Test '%s' did not succeed\n", path_to_string(&file->path));
    }
    string_free(&test_name);
    path_free(&test_path);
    return error;
}

bld_array project_tests_under(bld_project* project, bld_path* path) {
    bld_array tests;
    bld_file_id root_id;
    bld_file* root;

    root_id = os_info_id(path_to_string(path));
    if (root_id == BLD_INVALID_IDENITIFIER) {
        log_fatal(LOG_FATAL_PREFIX "path '%s' does not exist", path_to_string(path));
    }

    root = set_get(&project->files, root_id);
    if (root == NULL) {
        log_fatal(LOG_FATAL_PREFIX "requested path exists outside, or is ignored by, project");
    }

    tests = array_new(sizeof(bld_file));
    project_tests_under_recursive(&tests, root, &project->files);

    return tests;
}

void project_tests_under_recursive(bld_array* tests, bld_file* file, bld_set* files) {
    if (file->type == BLD_FILE_TEST) {
        array_push(tests, file);
    }

    if (file->type == BLD_FILE_DIRECTORY) {
        bld_iter iter;
        bld_file_id* sub_file_id;

        iter = iter_array(&file->info.dir.files);
        while (iter_next(&iter, (void**) &sub_file_id)) {
            bld_file* sub_file;
            sub_file = set_get(files, *sub_file_id);
            if (sub_file == NULL) {
                log_fatal(LOG_FATAL_PREFIX "internal error");
            }

            project_tests_under_recursive(tests, sub_file, files);
        }
    }
}
