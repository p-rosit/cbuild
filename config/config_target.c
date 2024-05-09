#include "../bld_core/logging.h"
#include "../bld_core/json.h"
#include "config_target.h"

void config_target_build_info_free(bld_target_build_information*);

void serialize_config_target_file(FILE*, bld_target_build_information*, int);

int parse_config_target_main(FILE*, bld_config_target*);
int parse_config_target_linker(FILE*, bld_config_target*);
int parse_config_target_files(FILE*, bld_config_target*);

int parse_target_build_info(FILE*, bld_target_build_information*);
int parse_target_build_info_file_name(FILE*, bld_target_build_information*);
int parse_target_build_info_file_compiler(FILE*, bld_target_build_information*);
int parse_target_build_info_file_compiler_flags(FILE*, bld_target_build_information*);
int parse_target_build_info_file_linker_flags(FILE*, bld_target_build_information*);
int parse_target_build_info_file_sub_files(FILE*, bld_target_build_information*);

bld_config_target config_target_new(bld_path* path) {
    bld_config_target config;
    config.path_main = *path;
    config.linker_set = 0;
    config.files_set = 0;
    return config;
}

void config_target_free(bld_config_target* config) {
    path_free(&config->path_main);
    if (config->linker_set) {
        linker_free(&config->linker);
    }
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
}

void serialize_config_target(bld_path* path, bld_config_target* config) {
    FILE* file = fopen(path_to_string(path), "w");
    int depth = 1;
    if (file == NULL) {
        log_fatal("Could not open target config file \"%s\"", path_to_string(path));
        return;
    }

    fprintf(file, "{\n");

    json_serialize_key(file, "main", depth);
    fprintf(file, "\"%s\"", path_to_string(&config->path_main));

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

    }

    if (info->info.linker_set) {
        fprintf(file, ",\n");
        json_serialize_key(file, "linker_flags", depth);
        serialize_linker_flags(file, &info->info.linker_flags, depth + 1);
    }

    if (info->files.size > 0) {
        int first = 1;
        bld_iter iter = iter_array(&info->files);
        bld_target_build_information* temp;

        fprintf(file, ",\n");
        json_serialize_key(file, "files", depth);
        fprintf(file, "[\n");
        while (iter_next(&iter, (void**) &temp)) {
            if (!first) {
                fprintf(file, ",\n");
            } else {
                first = 0;
            }
            fprintf(file, "%*c", 2 * (depth + 2), ' ');
            serialize_config_target_file(file, temp, depth + 2);
        }

        fprintf(file, "\n%*c]", 2 * (depth + 1), ' ');
    }

    fprintf(file, "\n%*c", 2 * depth, ' ');
    fprintf(file, "}");
}

int parse_config_target(bld_path* path, bld_config_target* config) {
    FILE* file = fopen(path_to_string(path), "r");
    int amount_parsed;
    int size = 3;
    int parsed[3];
    char *keys[3] = {"main", "linker", "files"};
    bld_parse_func funcs[3] = {
        (bld_parse_func) parse_config_target_main,
        (bld_parse_func) parse_config_target_linker,
        (bld_parse_func) parse_config_target_files,
    };

    if (file == NULL) {
        log_warn("Cannot read target config file with path: \"%s\"", path_to_string(path));
        return -1;
    }

    config->linker_set = 0;
    config->files_set = 0;
    amount_parsed = json_parse_map(file, config, size, parsed, keys, funcs);
    if (!parsed[0] || amount_parsed < 0) {
        log_warn("could not parse target config");
        if (parsed[0]) {
            path_free(&config->path_main);
        }

        if (parsed[1]) {
            linker_free(&config->linker);
        }

        if (parsed[2]) {
            config_target_build_info_free(&config->files);
        }
        return -1;
    }

    return 0;
}

int parse_config_target_main(FILE* file, bld_config_target* config) {
    bld_string path_main;
    int result = string_parse(file, &path_main);
    if (result) {
        log_warn("could not parse main file path");
        return -1;
    }

    config->path_main = path_from_string(string_unpack(&path_main));

    string_free(&path_main);
    return 0;
}

int parse_config_target_linker(FILE* file, bld_config_target* config) {
    bld_linker linker;
    int result = parse_linker(file, &linker);
    if (result) {
        log_warn("could not parse linker");
        return -1;
    }

    config->linker_set = 1;
    config->linker = linker;
    return 0;
}

int parse_config_target_files(FILE* file, bld_config_target* config) {
    bld_target_build_information files;
    int result = parse_target_build_info(file, &files);
    if (result) {
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
            bld_iter iter = iter_array(&files->files);
            bld_target_build_information* info;
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
    int result = string_parse(file, &name);
    if (result) {
        log_warn("could not parse file name");
        return -1;
    }
    info->name = name;
    return 0;
}

int parse_target_build_info_file_compiler(FILE* file, bld_target_build_information* info) {
    bld_compiler compiler;
    int result;

    if (info->info.compiler_set) {
        log_warn("compiler has already been parsed");
        return -1;
    }

    result = parse_compiler(file, &compiler);
    if (result) {
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
    int result;

    if (info->info.compiler_set) {
        log_warn("compiler bas already been parsed");
        return -1;
    }

    result = parse_compiler_flags(file, &flags);
    if (result) {
        log_warn("could not parse compiler flags");
    }

    info->info.compiler_set = 1;
    info->info.compiler.type = BLD_COMPILER_FLAGS;
    info->info.compiler.as.flags = flags;
    return 0;
}

int parse_target_build_info_file_linker_flags(FILE* file, bld_target_build_information* info) {
    bld_linker_flags flags;
    int result = parse_linker_flag(file, &flags);
    if (result) {
        log_warn("could not parse linker flags");
        return -1;
    }

    info->info.linker_set = 1;
    info->info.linker_flags = flags;
    return 0;
}

int parse_target_build_info_file_sub_files(FILE* file, bld_target_build_information* info) {
    bld_target_build_information sub_info;
    int result = parse_target_build_info(file, &sub_info);
    if (result) {
        log_warn("could not parse files under ");
        return -1;
    }

    array_push(&info->files, &sub_info);
    return 0;
}
