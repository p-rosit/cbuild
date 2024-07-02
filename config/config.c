#include "../bld_core/logging.h"
#include "../bld_core/json.h"
#include "config.h"

int parse_config_log_level(FILE*, bld_config*);
int parse_config_text_editor(FILE*, bld_config*);
int parse_config_default_target(FILE*, bld_config*);

bld_config config_new(void) {
    bld_config config;
    config.log_level = BLD_INFO;
    config.text_editor_configured = 0;
    config.active_target_configured = 0;
    return config;
}

void config_free(bld_config* config) {
    if (config->text_editor_configured) {
        string_free(&config->text_editor);
    }
    if (config->active_target_configured) {
        string_free(&config->active_target);
    }
}

void serialize_config(bld_path* path, bld_config* config) {
    FILE* file;
    int depth;

    file = fopen(path_to_string(path), "w");
    if (file == NULL) {
        log_fatal("Could not open config file \"%s\"", path_to_string(path));
        return;
    }

    depth = 1;

    fprintf(file, "{\n");

    json_serialize_key(file, "log_level", depth);
    fprintf(file, "\"%s\"", string_unpack(log_level_to_string(config->log_level)));

    if (config->text_editor_configured) {
        fprintf(file, ",\n");
        json_serialize_key(file, "text_editor", depth);
        fprintf(file, "\"%s\"", string_unpack(&config->text_editor));
    }

    if (config->active_target_configured) {
        fprintf(file, ",\n");
        json_serialize_key(file, "default_target", depth);
        fprintf(file, "\"%s\"", string_unpack(&config->active_target));
    }

    fprintf(file, "\n}");
    fclose(file);
}

int parse_config(bld_path* path, bld_config* config) {
    FILE* file;
    int amount_parsed;
    int size = 3;
    int parsed[3];
    char *keys[3] = {"log_level", "text_editor", "default_target"};
    bld_parse_func funcs[3] = {
        (bld_parse_func) parse_config_log_level,
        (bld_parse_func) parse_config_text_editor,
        (bld_parse_func) parse_config_default_target,
    };

    file = fopen(path_to_string(path), "r");
    if (file == NULL) {
        log_warn("Cannot read config file with path: \"%s\"", path_to_string(path));
        return -1;
    }

    config->text_editor_configured = 0;
    config->active_target_configured = 0;
    amount_parsed = json_parse_map(file, config, size, parsed, keys, funcs);
    if (amount_parsed < 0 || !parsed[0]) {
        log_warn("Could not parse project config");
        return -1;
    }

    fclose(file);
    return 0;
}

int parse_config_log_level(FILE* file, bld_config* config) {
    bld_string level;
    int error;

    error = string_parse(file, &level);
    if (error) {
        log_warn("Could not parse log level");
        return -1;
    }

    config->log_level = log_level_from_string(&level);

    string_free(&level);
    return config->log_level < 0;
}

int parse_config_text_editor(FILE* file, bld_config* config) {
    bld_string editor;
    int error;

    error = string_parse(file, &editor);
    if (error) {
        log_warn("Could not parse text editor");
        return -1;
    }

    config->text_editor_configured = 1;
    config->text_editor = editor;
    return 0;
}

int parse_config_default_target(FILE* file, bld_config* config) {
    bld_string target;
    int error;

    error = string_parse(file, &target);
    if (error) {
        log_warn("Could not parse default target");
        return -1;
    }
    config->active_target_configured = 1;
    config->active_target = target;
    return 0;
}
