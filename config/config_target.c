#include "../bld_core/logging.h"
#include "../bld_core/json.h"
#include "config_target.h"

int parse_config_target_main(FILE*, bld_config_target*);

bld_config_target config_target_new(bld_path* path) {
    bld_config_target config;
    config.path_main = *path;
    return config;
}

void config_target_free(bld_config_target* config) {
    path_free(&config->path_main);
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

    fprintf(file, "\n}");
    fclose(file);
}

int parse_config_target(bld_path* path, bld_config_target* config) {
    FILE* file = fopen(path_to_string(path), "r");
    int amount_parsed;
    int size = 1;
    int parsed[1];
    char *keys[1] = {"main"};
    bld_parse_func funcs[1] = {
        (bld_parse_func) parse_config_target_main,
    };

    if (file == NULL) {
        log_warn("Cannot read target config file with path: \"%s\"", path_to_string(path));
        return -1;
    }

    amount_parsed = json_parse_map(file, config, size, parsed, keys, funcs);
    if (amount_parsed < size) {
        log_warn("Could not parse target config");
        if (parsed[0]) {
            path_free(&config->path_main);
        }
        return -1;
    }

    return 0;
}

int parse_config_target_main(FILE* file, bld_config_target* config) {
    bld_string path_main;
    int result = string_parse(file, &path_main);
    if (result) {
        log_warn("Could not parse main file path");
        return -1;
    }

    config->path_main = path_from_string(string_unpack(&path_main));

    string_free(&path_main);
    return 0;
}
