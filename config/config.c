#include "../bld_core/logging.h"
#include "../bld_core/json.h"
#include "config.h"

int parse_config_text_editor(FILE*, bld_config*);

bld_config config_new(void) {
    bld_config config;
    config.text_editor_configured = 0;
    return config;
}

void config_free(bld_config* config) {
    if (config->text_editor_configured) {
        string_free(&config->text_editor);
    }
}

void serialize_config(bld_path* path, bld_config* config) {
    FILE* file = fopen(path_to_string(path), "w");
    int depth = 1;
    if (file == NULL) {
        log_fatal("Could not open config file \"%s\"", path_to_string(path));
        return;
    }

    fprintf(file, "{\n");

    if (config->text_editor_configured) {
        json_serialize_key(file, "text_editor", depth);
        fprintf(file, "\"%s\"", string_unpack(&config->text_editor));
        fprintf(file, "\n");
    }

    fprintf(file, "\n}");
    fclose(file);
}

int parse_config(bld_path* path, bld_config* config) {
    FILE* file = fopen(path_to_string(path), "r");
    int amount_parsed;
    int size = 1;
    int parsed[1];
    char *keys[1] = {"text_editor"};
    bld_parse_func funcs[1] = {
        (bld_parse_func) parse_config_text_editor,
    };

    if (file == NULL) {
        log_warn("Cannot write to config file with path: \"%s\"", path_to_string(path));
        return -1;
    }

    config->text_editor_configured = 0;
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
