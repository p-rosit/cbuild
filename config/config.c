#include "../bld_core/logging.h"
#include "../bld_core/json.h"
#include "config.h"

int parse_config_text_editor(FILE*, bld_config*);
int parse_config_default_target(FILE*, bld_config*);

bld_config config_new(void) {
    bld_config config;
    config.text_editor_configured = 0;
    config.default_target_configured = 0;
    return config;
}

void config_free(bld_config* config) {
    if (config->text_editor_configured) {
        string_free(&config->text_editor);
    }
    if (config->default_target_configured) {
        string_free(&config->target);
    }
}

void serialize_config(bld_path* path, bld_config* config) {
    FILE* file = fopen(path_to_string(path), "w");
    int depth = 1;
    int first = 1;
    if (file == NULL) {
        log_fatal("Could not open config file \"%s\"", path_to_string(path));
        return;
    }

    fprintf(file, "{\n");

    if (config->text_editor_configured) {
        if (!first) {fprintf(file, ",\n");}

        json_serialize_key(file, "text_editor", depth);
        fprintf(file, "\"%s\"", string_unpack(&config->text_editor));

        first = 0;
    }

    if (config->default_target_configured) {
        if (!first) {fprintf(file, ",\n");}

        json_serialize_key(file, "default_target", depth);
        fprintf(file, "\"%s\"", string_unpack(&config->target));

        first = 0;
    }

    fprintf(file, "\n}");
    fclose(file);
}

int parse_config(bld_path* path, bld_config* config) {
    FILE* file = fopen(path_to_string(path), "r");
    int amount_parsed;
    int size = 2;
    int parsed[2];
    char *keys[2] = {"text_editor", "default_target"};
    bld_parse_func funcs[2] = {
        (bld_parse_func) parse_config_text_editor,
        (bld_parse_func) parse_config_default_target,
    };

    if (file == NULL) {
        log_warn("Cannot write to config file with path: \"%s\"", path_to_string(path));
        return -1;
    }

    config->text_editor_configured = 0;
    config->default_target_configured = 0;
    amount_parsed = json_parse_map(file, config, size, parsed, keys, funcs);
    if (amount_parsed < 0) {
        log_warn("Could not parse project config");
        return -1;
    }

    fclose(file);
    return 0;
}

int parse_config_text_editor(FILE* file, bld_config* config) {
    bld_string editor;
    int result = string_parse(file, &editor);
    if (result) {
        log_warn("Could not parse text editor");
        return -1;
    }
    config->text_editor_configured = 1;
    config->text_editor = editor;
    return 0;
}

int parse_config_default_target(FILE* file, bld_config* config) {
    bld_string target;
    int result = string_parse(file, &target);
    if (result) {
        log_warn("Could not parse default target");
        return -1;
    }
    config->default_target_configured= 1;
    config->target= target;
    return 0;
}
