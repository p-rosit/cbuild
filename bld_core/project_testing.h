#ifndef PROJECT_TESTING_H
#define PROJECT_TESTING_H
#include "project.h"

int project_test_files(bld_project*, bld_array*);
int project_test_file(bld_project*, bld_file*);
bld_array project_tests_under(bld_project*, bld_path*);

#endif
