#include <stdio.h>
#include <inttypes.h>
#include "logging.h"
#include "project.h"

void serialize_compiler(FILE*, bld_compiler*, int);
void serialize_compiler_type(FILE*, bld_compiler_type);
void serialize_compiler_flags(FILE*, bld_array*, int);

void serialize_files(FILE*, bld_set*, bld_array*, int);
void serialize_file(FILE*, bld_file*, bld_array*, int);
void serialize_file_type(FILE*, bld_file_type);
void serialize_file_id(FILE*, bld_file_identifier);
void serialize_file_symbols(FILE*, bld_set*, int);
void serialize_file_includes(FILE*, bld_set*, int);

void serialize_key(FILE*, char*, int);

void project_save_cache(bld_project* project) {
    FILE* cache;
    bld_path cache_path;
    int depth = 1;

    if (!project->base.cache.loaded) {
        log_warn("Trying to save cache but no cache was loaded. Ignoring.");
        return;
    }

    cache_path = path_copy(&project->base.root);
    path_append_path(&cache_path, &project->base.cache.root);
    path_append_string(&cache_path, BLD_CACHE_NAME);

    cache = fopen(path_to_string(&cache_path), "w");
    if (cache == NULL) {
        log_fatal("Could not open cache file for writing under: \"%s\"", path_to_string(&cache_path));
    }

    fprintf(cache, "{\n");
    serialize_key(cache, "compiler", depth);
    serialize_compiler(cache, &project->base.compiler, depth + 1);
    fprintf(cache, ",\n");

    serialize_key(cache, "files", depth);
    serialize_files(cache, &project->files, &project->base.file_compilers, depth + 1);
    fprintf(cache, "\n");

    fprintf(cache, "}\n");

    fclose(cache);
    path_free(&cache_path);
}

void serialize_compiler(FILE* cache, bld_compiler* compiler, int depth) {
    fprintf(cache, "{\n");
    
    serialize_key(cache, "type", depth);
    serialize_compiler_type(cache, compiler->type);
    fprintf(cache, ",\n");

    serialize_key(cache, "executable", depth);
    fprintf(cache, "\"%s\"", compiler->executable);

    if (compiler->flags.size > 0) {
        fprintf(cache, ",\n");
        serialize_key(cache, "flags", depth);
        serialize_compiler_flags(cache, &compiler->flags, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_compiler_type(FILE* cache, bld_compiler_type type) {
    switch (type) {
        case (BLD_GCC): {
            fprintf(cache, "\"gcc\"");
        } break;
        case (BLD_CLANG): {
            fprintf(cache, "\"clang\"");
        } break;
        default: log_fatal("serialize_compiler_type: unreachable error???");
    }
}

void serialize_compiler_flags(FILE* cache, bld_array* flags, int depth) {
    char** flag;
    int first = 1;

    fprintf(cache, "[\n");
    
    bld_iter iter = iter_array(flags);
    while (iter_next(&iter, (void**) &flag)) {
        if (!first) {fprintf(cache, ",\n");}
        else {first = 0;}
        fprintf(cache, "%*c\"%s\"", 2 * depth, ' ', *flag);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * (depth - 1), ' ');
}

void serialize_files(FILE* cache, bld_set* files, bld_array* compilers, int depth) {
    int first = 1;
    bld_iter iter = iter_set(files);
    bld_file* file;

    fprintf(cache, "[\n");
    while (iter_next(&iter, (void**) &file)) {
        if (!first) {
            fprintf(cache, ",\n");
        } else {
            first = 0;
        }
        fprintf(cache, "%*c", 2 * depth, ' ');
        serialize_file(cache, file, compilers, depth + 1);
    }
    fprintf(cache, "\n");
    fprintf(cache, "%*c]", 2 * (depth - 1), ' ');
}

void serialize_file(FILE* cache, bld_file* file, bld_array* compilers, int depth) {
    fprintf(cache, "{\n");

    serialize_key(cache, "type", depth);
    serialize_file_type(cache, file->type);
    fprintf(cache, ",\n");

    serialize_key(cache, "id", depth);
    serialize_file_id(cache, file->identifier);
    fprintf(cache, ",\n");

    serialize_key(cache, "hash", depth);
    fprintf(cache, "%ju", file->identifier.hash);
    fprintf(cache, ",\n");

    serialize_key(cache, "name", depth);
    fprintf(cache, "\"%s\"", string_unpack(&file->name));
    fprintf(cache, ",\n");

    if (file->compiler > 0) {
        serialize_key(cache, "compiler", depth);
        serialize_compiler(cache, array_get(compilers, file->compiler), depth + 1);
        fprintf(cache, ",\n");
    }

    serialize_key(cache, "includes", depth);
    serialize_file_includes(cache, &file->includes, depth + 1);

    if (file->type != BLD_HEADER) {
        fprintf(cache, ",\n");

        serialize_key(cache, "defined_symbols", depth);
        serialize_file_symbols(cache, &file->defined_symbols, depth + 1);
        fprintf(cache, ",\n");

        serialize_key(cache, "undefined_symbols", depth);
        serialize_file_symbols(cache, &file->undefined_symbols, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
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

void serialize_file_symbols(FILE* cache, bld_set* symbols, int depth) {
    int first = 1;
    char** symbol;
    bld_iter iter = iter_set(symbols);

    fprintf(cache, "[\n");
    while (iter_next(&iter, (void**) &symbol)) {
        if (!first) {
            fprintf(cache, ",\n");
        } else {
            first = 0;
        }
        fprintf(cache, "%*c\"%s\"", 2 * depth, ' ', *symbol);
    }
    fprintf(cache, "\n%*c]", 2 * (depth - 1), ' ');
}

void serialize_file_includes(FILE* cache, bld_set* includes, int depth) {
    int first = 1;
    fprintf(cache, "[\n");
    for (size_t i = 0; i < includes->capacity + includes->max_offset; i++) {
        if (includes->offset[i] >= includes->max_offset) {continue;}
        if (!first) {
            fprintf(cache, ",\n");
        } else {
            first = 0;
        }
        fprintf(cache, "%*c%ju", 2 * depth, ' ', includes->hash[i]);
    }
    fprintf(cache, "\n%*c]", 2 * (depth - 1), ' ');
}

void serialize_key(FILE* cache, char* key, int depth) {
    fprintf(cache, "%*c\"%s\": ", 2 * depth, ' ', key);
}
