#include "../bld_core/logging.h"
#include "../bld_core/json.h"
#include "config_target.h"

void serialize_config_target_file(FILE*, bld_target_build_information*, int);

int parse_config_target_main(FILE*, bld_config_target*);
int parse_config_target_linker(FILE*, bld_config_target*);
int parse_config_target_files(FILE*, bld_config_target*);
int parse_config_target_added_paths(FILE*, bld_config_target*);
int parse_config_target_ignored_paths(FILE*, bld_config_target*);
int parse_config_target_paths(FILE*, bld_array*);
int parse_config_target_path(FILE*, bld_array*);

int parse_target_build_info(FILE*, bld_target_build_information*);
int parse_target_build_info_file_name(FILE*, bld_target_build_information*);
int parse_target_build_info_file_compiler(FILE*, bld_target_build_information*);
int parse_target_build_info_file_compiler_flags(FILE*, bld_target_build_information*);
int parse_target_build_info_file_linker_flags(FILE*, bld_target_build_information*);
int parse_target_build_info_file_sub_files(FILE*, bld_target_build_information*);
int parse_target_build_info_file_sub_file(FILE*, bld_array*);

void config_target_extract_compiler_types(bld_config_target*, bld_target_build_information*);

bld_config_target config_target_new(bld_path* path) {
    bld_config_target config;
    config.path_main = *path;
    config.added_paths = array_new(sizeof(bld_path));
    config.ignore_paths = array_new(sizeof(bld_path));
    config.linker_set = 0;
    config.compiler_types = set_new(sizeof(bld_compiler_type));
    config.files_set = 0;
    return config;
}

void config_target_free(bld_config_target* config) {
    bld_path* path;
    bld_iter iter;
    path_free(&config->path_main);

    iter = iter_array(&config->added_paths);
    while (iter_next(&iter, (void**) &path)) {
        path_free(path);
    }
    array_free(&config->added_paths);

    iter = iter_array(&config->ignore_paths);
    while (iter_next(&iter, (void**) &path)) {
        path_free(path);
    }
    array_free(&config->ignore_paths);

    if (config->linker_set) {
        linker_free(&config->linker);
    }
    set_free(&config->compiler_types);
    if (config->files_set) {
        config_target_build_info_free(&config->files);
    }
}

void config_target_build_info_free(bld_target_build_information* info) {
    bld_iter iter;
    bld_target_build_information* temp;

    string_free(&info->name);
    file_build_info_free(&info->info);

    iter = iter_array(&info->files);
    while (iter_next(&iter, (void**) &temp)) {
        config_target_build_info_free(temp);
    }
    array_free(&info->files);
}

void serialize_config_target(bld_path* path, bld_config_target* config) {
    FILE* file;
    int depth;

    file = fopen(path_to_string(path), "w");
    if (file == NULL) {
        log_fatal("Could not open target config file \"%s\"", path_to_string(path));
        return;
    }

    depth = 1;
    fprintf(file, "{\n");

    json_serialize_key(file, "main", depth);
    fprintf(file, "\"%s\"", path_to_string(&config->path_main));

    fprintf(file, ",\n");
    json_serialize_key(file, "added_paths", depth);
    {
        bld_iter iter;
        bld_path* path;
        int first;

        first = 1;
        fprintf(file, "[");
        iter = iter_array(&config->added_paths);
        while (iter_next(&iter, (void**) &path)) {
            if (!first) {
                fprintf(file, ",");
            }
            first = 0;
            fprintf(file, "\n%*c", 2 * (depth + 1), ' ');
            fprintf(file, "\"%s\"", path_to_string(path));
        }
        if (config->added_paths.size > 0) {
            fprintf(file, "\n%*c", 2 * depth, ' ');
        }
        fprintf(file, "]");
    }

    fprintf(file, ",\n");
    json_serialize_key(file, "ignore_paths", depth);
    {
        bld_iter iter;
        bld_path* path;
        int first;

        first = 1;
        fprintf(file, "[");
        iter = iter_array(&config->ignore_paths);
        while (iter_next(&iter, (void**) &path)) {
            if (!first) {
                fprintf(file, ",");
            }
            first = 0;
            fprintf(file, "\n%*c", 2 * (depth + 1), ' ');
            fprintf(file, "\"%s\"", path_to_string(path));
        }
        if (config->ignore_paths.size > 0) {
            fprintf(file, "\n%*c", 2 * depth, ' ');
        }
        fprintf(file, "]");
    }

    if (config->linker_set) {
        fprintf(file, ",\n");
        json_serialize_key(file, "linker", depth);
        serialize_linker(file, &config->linker, depth + 1);
    }

    if (config->files_set) {
        fprintf(file, ",\n");
        json_serialize_key(file, "files", depth);
        serialize_config_target_file(file, &config->files, depth + 1);
    }

    fprintf(file, "\n}");
    fclose(file);
}

void serialize_config_target_file(FILE* file, bld_target_build_information* info, int depth) {
    fprintf(file, "{\n");

    json_serialize_key(file, "name", depth);
    fprintf(file, "\"%s\"", string_unpack(&info->name));

    if (info->info.compiler_set) {
        fprintf(file, ",\n");
        switch (info->info.compiler.type) {
            case (BLD_COMPILER): {
                json_serialize_key(file, "compiler", depth);
                serialize_compiler(file, &info->info.compiler.as.compiler, depth + 1);
            } break;
            case (BLD_COMPILER_FLAGS): {
                if (info->info.compiler.as.flags.flags.size > 0 || info->info.compiler.as.flags.removed.size > 0) {
                    json_serialize_key(file, "compiler_flags", depth);
                    serialize_compiler_flags(file, &info->info.compiler.as.flags, depth + 1);
                }
            } break;
        }
    }

    if (info->info.linker_set) {
        if (info->info.linker_flags.flags.size > 0) {
            fprintf(file, ",\n");
            json_serialize_key(file, "linker_flags", depth);
            serialize_linker_flags(file, &info->info.linker_flags, depth + 1);
        }
    }

    if (info->files.size > 0) {
        int first;
        bld_iter iter;
        bld_target_build_information* temp;

        fprintf(file, ",\n");

        json_serialize_key(file, "files", depth);
        fprintf(file, "[\n");
        first = 1;
        iter = iter_array(&info->files);
        while (iter_next(&iter, (void**) &temp)) {
            if (!first) {
                fprintf(file, ",\n");
            } else {
                first = 0;
            }
            fprintf(file, "%*c", 2 * (depth + 1), ' ');
            serialize_config_target_file(file, temp, depth + 2);
        }

        fprintf(file, "\n%*c]", 2 * depth, ' ');
    }

    fprintf(file, "\n%*c", 2 * (depth - 1), ' ');
    fprintf(file, "}");
}

int parse_config_target(bld_path* path, bld_config_target* config) {
    FILE* file;
    int amount_parsed;
    int size = 5;
    int parsed[5];
    char *keys[5] = {"main", "added_paths", "ignore_paths", "linker", "files"};
    bld_parse_func funcs[5] = {
        (bld_parse_func) parse_config_target_main,
        (bld_parse_func) parse_config_target_added_paths,
        (bld_parse_func) parse_config_target_ignored_paths,
        (bld_parse_func) parse_config_target_linker,
        (bld_parse_func) parse_config_target_files,
    };

    file = fopen(path_to_string(path), "r");
    if (file == NULL) {
        log_warn("Cannot read target config file with path: \"%s\"", path_to_string(path));
        return -1;
    }

    config->linker_set = 0;
    config->compiler_types = set_new(sizeof(bld_compiler_type));
    config->files_set = 0;
    config->added_paths = array_new(sizeof(bld_path));
    config->ignore_paths = array_new(sizeof(bld_path));
    amount_parsed = json_parse_map(file, config, size, parsed, keys, funcs);
    if (!parsed[0] || !parsed[1] || amount_parsed < 0) {
        log_warn("could not parse target config");

        if (parsed[0]) {
            path_free(&config->path_main);
        }

        if (parsed[1]) {
            bld_iter iter;
            bld_path* path;

            iter = iter_array(&config->added_paths);
            while (iter_next(&iter, (void**) &path)) {
                path_free(path);
            }
            array_free(&config->ignore_paths);
        }

        if (parsed[2]) {
            bld_iter iter;
            bld_path* path;

            iter = iter_array(&config->ignore_paths);
            while (iter_next(&iter, (void**) &path)) {
                path_free(path);
            }
            array_free(&config->ignore_paths);
        }

        if (parsed[3]) {
            linker_free(&config->linker);
        }

        if (parsed[4]) {
            config_target_build_info_free(&config->files);
        }
        return -1;
    }

    config_target_extract_compiler_types(config, &config->files);

    return 0;
}

void config_target_extract_compiler_types(bld_config_target* config, bld_target_build_information* info) {
    bld_iter iter;
    bld_target_build_information* child;

    if (!info->info.compiler_set) {
        goto not_adding_handle;
    }
    if (info->info.compiler.type != BLD_COMPILER) {
        goto not_adding_handle;
    }
    if (set_has(&config->compiler_types, info->info.compiler.as.compiler.type)) {
        goto not_adding_handle;
    }

    set_add(&config->compiler_types, info->info.compiler.as.compiler.type, &info->info.compiler.as.compiler.type);
    not_adding_handle:

    iter = iter_array(&info->files);
    while (iter_next(&iter, (void**) &child)) {
        config_target_extract_compiler_types(config, child);
    }
}

int parse_config_target_main(FILE* file, bld_config_target* config) {
    bld_string path_main;
    int error;

    error = string_parse(file, &path_main);
    if (error) {
        log_warn("could not parse main file path");
        return -1;
    }

    config->path_main = path_from_string(string_unpack(&path_main));

    string_free(&path_main);
    return 0;
}

int parse_config_target_added_paths(FILE* file, bld_config_target* config) {
    return parse_config_target_paths(file, &config->added_paths);
}

int parse_config_target_ignored_paths(FILE* file, bld_config_target* config) {
    return parse_config_target_paths(file, &config->ignore_paths);
}

int parse_config_target_paths(FILE* file, bld_array* array) {
    int amount_parsed;

    amount_parsed = json_parse_array(file, array, (bld_parse_func) parse_config_target_path);
    if (amount_parsed < 0) {
        bld_iter iter;
        bld_path* path;

        iter = iter_array(array);
        while (iter_next(&iter, (void**) &path)) {
            path_free(path);
        }
        array_free(array);
        return -1;
    }
    return 0;
}

int parse_config_target_path(FILE* file, bld_array* array) {
    bld_string temp;
    bld_path path;
    int error;

    error = string_parse(file, &temp);
    if (error) {
        return -1;
    }

    path = path_from_string(string_unpack(&temp));
    array_push(array, &path);
    string_free(&temp);
    return 0;
}

int parse_config_target_linker(FILE* file, bld_config_target* config) {
    bld_linker linker;
    int error;

    error = parse_linker(file, &linker);
    if (error) {
        log_warn("could not parse linker");
        return -1;
    }

    config->linker_set = 1;
    config->linker = linker;
    return 0;
}

int parse_config_target_files(FILE* file, bld_config_target* config) {
    bld_target_build_information files;
    int error;

    error = parse_target_build_info(file, &files);
    if (error) {
        log_warn("could not parse files");
        return -1;
    }

    config->files_set = 1;
    config->files = files;
    return 0;
}

int parse_target_build_info(FILE* file, bld_target_build_information* files) {
    int amount_parsed;
    int size = 5;
    int parsed[5];
    char *keys[5] = {"name", "compiler", "compiler_flags", "linker_flags", "files"};
    bld_parse_func funcs[5] = {
        (bld_parse_func) parse_target_build_info_file_name,
        (bld_parse_func) parse_target_build_info_file_compiler,
        (bld_parse_func) parse_target_build_info_file_compiler_flags,
        (bld_parse_func) parse_target_build_info_file_linker_flags,
        (bld_parse_func) parse_target_build_info_file_sub_files,
    };

    files->info.compiler_set = 0;
    files->info.linker_set = 0;
    files->files = array_new(sizeof(bld_target_build_information));
    amount_parsed = json_parse_map(file, files, size, parsed, keys, funcs);
    if (!parsed[0] || amount_parsed < 0) {
        log_warn("could not parse file tree");

        if (parsed[0]) {
            string_free(&files->name);
        }

        if (parsed[1]) {
            compiler_free(&files->info.compiler.as.compiler);
        }
        
        if (parsed[2]) {
            compiler_flags_free(&files->info.compiler.as.flags);
        }

        if (parsed[3]) {
            linker_flags_free(&files->info.linker_flags);
        }

        if (parsed[4]) {
            bld_iter iter;
            bld_target_build_information* info;

            iter = iter_array(&files->files);
            while (iter_next(&iter, (void**) &info)) {
                config_target_build_info_free(info);
            }
        }

        return -1;
    }

    return 0;
}

int parse_target_build_info_file_name(FILE* file, bld_target_build_information* info) {
    bld_string name;
    int error;

    error = string_parse(file, &name);
    if (error) {
        log_warn("could not parse file name");
        return -1;
    }

    info->name = name;
    return 0;
}

int parse_target_build_info_file_compiler(FILE* file, bld_target_build_information* info) {
    bld_compiler compiler;
    int error;

    if (info->info.compiler_set) {
        log_warn("compiler has already been parsed");
        return -1;
    }

    error = parse_compiler(file, &compiler);
    if (error) {
        log_warn("could not parse compiler");
        return -1;
    }

    info->info.compiler_set = 1;
    info->info.compiler.type = BLD_COMPILER;
    info->info.compiler.as.compiler = compiler;
    return 0;
}

int parse_target_build_info_file_compiler_flags(FILE* file, bld_target_build_information* info) {
    bld_compiler_flags flags;
    int error;

    if (info->info.compiler_set) {
        log_warn("compiler has already been parsed");
        return -1;
    }

    error = parse_compiler_flags(file, &flags);
    if (error) {
        log_warn("could not parse compiler flags");
    }

    info->info.compiler_set = 1;
    info->info.compiler.type = BLD_COMPILER_FLAGS;
    info->info.compiler.as.flags = flags;
    return 0;
}

int parse_target_build_info_file_linker_flags(FILE* file, bld_target_build_information* info) {
    bld_linker_flags flags;
    int error;

    error = parse_linker_flags(file, &flags);
    if (error) {
        log_warn("could not parse linker flags");
        return -1;
    }

    info->info.linker_set = 1;
    info->info.linker_flags = flags;
    return 0;
}

int parse_target_build_info_file_sub_files(FILE* file, bld_target_build_information* info) {
    bld_array sub_files;
    int amount_parsed;

    sub_files = array_new(sizeof(bld_target_build_information));
    amount_parsed = json_parse_array(file, &sub_files, (bld_parse_func) parse_target_build_info_file_sub_file);

    if (amount_parsed < 0) {
        log_warn("could not parse files under \"%s\"", string_unpack(&info->name));
        log_fatal("Free correctly!");
        return -1;
    }

    array_free(&info->files);
    info->files = sub_files;

    return 0;
}

int parse_target_build_info_file_sub_file(FILE* file, bld_array* files) {
    bld_target_build_information info;
    int error;

    error = parse_target_build_info(file, &info);
    if (error) {
        log_warn("could not parse file");
        return -1;
    }

    array_push(files, &info);
    return 0;
}
