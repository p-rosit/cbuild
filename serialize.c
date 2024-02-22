#include "build.h"
#include "project.h"

void serialize_compiler(FILE*, bld_compiler*, int);
void serialize_compiler_type(FILE*, bld_compiler_type);
void serialize_files(FILE*, bld_files*, int);
void serialize_file(FILE*, bld_file*, int);
void serialize_file_type(FILE*, bld_file_type);
void serialize_file_id(FILE*, bld_file_identifier);

void save_cache(bld_project project) {
    FILE* cache;
    bld_path cache_path;

    if (project.cache == NULL) {
        log_warn("Trying to save cache but no cache was loaded. Ignoring.");
        return;
    }

    cache_path = copy_path(&project.root);
    append_path(&cache_path, &(**project.cache).path);
    append_dir(&cache_path, BLD_CACHE_NAME);

    cache = fopen(path_to_string(&cache_path), "w");
    if (cache == NULL) {
        log_fatal("Could not open cache file for writing under: \"%s\"", path_to_string(&cache_path));
    }

    fprintf(cache, "{\n");
    fprintf(cache, "  \"compiler\": ");
    serialize_compiler(cache, project.compiler, 1);
    fprintf(cache, ",\n");
    fprintf(cache, "  \"files\": ");
    serialize_files(cache, project.files, 1);
    fprintf(cache, "\n}\n");

    fclose(cache);
    free_path(&cache_path);


    log_warn("save_cache: unfinished.");
}

void serialize_compiler(FILE* cache, bld_compiler* compiler, int depth) {
    fprintf(cache, "{\n");
    fprintf(cache, "%*c}", 2 * depth, ' ');
}

void serialize_files(FILE* cache, bld_files* files, int depth) {
    fprintf(cache, "[\n");
    for (size_t i = 0; i < files->size; i++) {
        if (i > 0) {
            fprintf(cache, ",\n");
        }
        fprintf(cache, "%*c", 2 * (depth + 1), ' ');
        serialize_file(cache, &files->files[i], depth + 1);
    }
    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * depth, ' ');
}

void serialize_file(FILE* cache, bld_file* file, int depth) {
    fprintf(cache, "{\n");

    fprintf(cache, "%*c\"type\": ", 2 * (depth + 1), ' ');
    serialize_file_type(cache, file->type);
    fprintf(cache, ",\n");

    fprintf(cache, "%*c\"id\": ", 2 * (depth + 1), ' ');
    serialize_file_id(cache, file->identifier);
    fprintf(cache, ",\n");

    fprintf(cache, "%*c\"name\": \"%s\"", 2 * (depth + 1), ' ', make_string(&file->name));

    if (file->compiler != NULL) {
        fprintf(cache, ",\n");
        fprintf(cache, "%*c\"compiler\": ", 2 * (depth + 1), ' ');
        serialize_compiler(cache, file->compiler, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * depth, ' ');
}

void serialize_file_type(FILE* cache, bld_file_type type) {
    switch (type) {
        case (BLD_IMPL): {
            fprintf(cache, "\"implementation\"");
        } break;
        case (BLD_HEADER): {
            fprintf(cache, "\"header\"");
        } break;
        case (BLD_TEST): {
            fprintf(cache, "\"test\"");
        } break;
        default: log_fatal("serialize_file_type: unreachable error???");
    }
}

void serialize_file_id(FILE* cache, bld_file_identifier id) {
    fprintf(cache, "%ju", id.id);
}
