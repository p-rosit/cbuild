#include <stdio.h>
#include <inttypes.h>
#include "logging.h"
#include "project.h"

void serialize_compiler(FILE*, bld_compiler*, int);
void serialize_compiler_flags(FILE*, bld_array*, int);
void serialize_linker(FILE*, bld_linker*, int);
void serialize_linker_flags(FILE*, bld_linker_flags*, int);

void serialize_files(FILE*, bld_set*, bld_array*, bld_array*, int);
void serialize_file(FILE*, bld_file*, bld_array*, bld_array*, int);
void serialize_file_type(FILE*, bld_file_type);
void serialize_file_id(FILE*, bld_file_identifier);
void serialize_file_mtime(FILE*, bld_file_identifier);
void serialize_file_symbols(FILE*, bld_set*, int);
void serialize_file_includes(FILE*, bld_set*, int);

void serialize_key(FILE*, char*, int);

void project_save_cache(bld_project* project) {
    FILE* cache;
    bld_path cache_path;
    int depth = 1;

    if (!project->base.cache.loaded) {
        log_fatal("Trying to save cache without a corresponding load cache, i.e. no cache path has been set.");
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

    serialize_key(cache, "linker", depth);
    serialize_linker(cache, &project->base.linker, depth + 1);
    fprintf(cache, ",\n");

    serialize_key(cache, "files", depth);
    serialize_files(cache, &project->files, &project->base.file_compilers, &project->base.file_linker_flags, depth + 1);
    fprintf(cache, "\n");

    fprintf(cache, "}\n");

    fclose(cache);
    path_free(&cache_path);
}

void serialize_compiler(FILE* cache, bld_compiler* compiler, int depth) {
    fprintf(cache, "{\n");

    serialize_key(cache, "executable", depth);
    fprintf(cache, "\"%s\"", string_unpack(&compiler->executable));

    if (compiler->flags.size > 0) {
        fprintf(cache, ",\n");
        serialize_key(cache, "flags", depth);
        serialize_compiler_flags(cache, &compiler->flags, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_compiler_flags(FILE* cache, bld_array* flags, int depth) {
    int first = 1;
    bld_iter iter;
    bld_string* flag;

    fprintf(cache, "[");
    if (flags->size > 1) {
        fprintf(cache, "\n");
    }
    
    iter = iter_array(flags);
    while (iter_next(&iter, (void**) &flag)) {
        if (!first) {fprintf(cache, ",\n");}
        else {first = 0;}
        if (flags->size > 1) {
            fprintf(cache, "%*c", 2 * depth, ' ');
        }
        fprintf(cache, "\"%s\"", string_unpack(flag));
    }

    if (flags->size > 1) {
        fprintf(cache, "\n%*c", 2 * (depth - 1), ' ');
    }
    fprintf(cache, "]");
}

void serialize_linker(FILE* cache, bld_linker* linker, int depth) {
    fprintf(cache, "{\n");

    serialize_key(cache, "executable", depth);
    fprintf(cache, "\"%s\"", linker->executable);

    if (linker->flags.flag.size > 0) {
        fprintf(cache, ",\n");
        serialize_key(cache, "linker_flags", depth);
        serialize_linker_flags(cache, &linker->flags, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_linker_flags(FILE* cache, bld_linker_flags* flags, int depth) {
    char** flag;
    int first = 1;
    bld_iter iter;

    fprintf(cache, "[");
    if (flags->flag.size > 1) {
        fprintf(cache, "\n");
    }
    
    iter = iter_array(&flags->flag);
    while (iter_next(&iter, (void**) &flag)) {
        if (!first) {fprintf(cache, ",\n");}
        else {first = 0;}
        if (flags->flag.size > 1) {
            fprintf(cache, "%*c", 2 * depth, ' ');
        }
        fprintf(cache, "\"%s\"", *flag);
    }

    if (flags->flag.size > 1) {
        fprintf(cache, "\n%*c", 2 * (depth - 1), ' ');
    }
    fprintf(cache, "]");
}

void serialize_files(FILE* cache, bld_set* files, bld_array* compilers, bld_array* linker_flags, int depth) {
    int first = 1;
    bld_iter iter = iter_set(files);
    bld_file* file;

    fprintf(cache, "[");
    if (files->size > 1) {
        fprintf(cache, "\n");
    }

    while (iter_next(&iter, (void**) &file)) {
        if (!first) {
            fprintf(cache, ",\n");
        } else {
            first = 0;
        }

        if (files->size > 1) {
            fprintf(cache, "%*c", 2 * depth, ' ');
        }
        serialize_file(cache, file, compilers, linker_flags, depth + 1);
    }

    if (files->size > 1) {
        fprintf(cache, "\n%*c", 2 * (depth - 1), ' ');
    }
    fprintf(cache, "]");
}

void serialize_file(FILE* cache, bld_file* file, bld_array* compilers, bld_array* linker_flags, int depth) {
    fprintf(cache, "{\n");

    serialize_key(cache, "type", depth);
    serialize_file_type(cache, file->type);
    fprintf(cache, ",\n");

    serialize_key(cache, "id", depth);
    serialize_file_id(cache, file->identifier);
    fprintf(cache, ",\n");

    serialize_key(cache, "mtime", depth);
    serialize_file_mtime(cache, file->identifier);
    fprintf(cache, ",\n");

    serialize_key(cache, "hash", depth);
    fprintf(cache, "%" PRIuMAX, file->identifier.hash);
    fprintf(cache, ",\n");

    serialize_key(cache, "name", depth);
    fprintf(cache, "\"%s\"", string_unpack(&file->name));
    fprintf(cache, ",\n");

    if (file->compiler >= 0) {
        serialize_key(cache, "compiler", depth);
        serialize_compiler(cache, array_get(compilers, file->compiler), depth + 1);
        fprintf(cache, ",\n");
    }

    if (file->linker_flags >= 0) {
        serialize_key(cache, "linker_flags", depth);
        serialize_linker_flags(cache, array_get(linker_flags, file->linker_flags), depth + 1);
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
    fprintf(cache, "%" PRIuMAX, id.id);
}

void serialize_file_mtime(FILE* cache, bld_file_identifier id) {
    fprintf(cache, "%" PRIuMAX, id.time);
}

void serialize_file_symbols(FILE* cache, bld_set* symbols, int depth) {
    int first = 1;
    char** symbol;
    bld_iter iter = iter_set(symbols);

    fprintf(cache, "[");
    if (symbols->size > 1) {
        fprintf(cache, "\n");
    }

    while (iter_next(&iter, (void**) &symbol)) {
        if (!first) {
            fprintf(cache, ",\n");
        } else {
            first = 0;
        }
        if (symbols->size > 1) {
            fprintf(cache, "%*c", 2 * depth, ' ');
        }
        fprintf(cache, "\"%s\"", *symbol);
    }

    if (symbols->size > 1) {
        fprintf(cache, "\n%*c", 2 * (depth - 1), ' ');
    }
    fprintf(cache, "]");
}

void serialize_file_includes(FILE* cache, bld_set* includes, int depth) {
    int first = 1;
    size_t i;

    fprintf(cache, "[");
    if (includes->size > 1) {
        fprintf(cache, "\n");
    }

    for (i = 0; i < includes->capacity + includes->max_offset; i++) {
        if (includes->offset[i] >= includes->max_offset) {continue;}
        if (!first) {
            fprintf(cache, ",\n");
        } else {
            first = 0;
        }
        if (includes->size > 1) {
            fprintf(cache, "%*c", 2 * depth, ' ');
        }
        fprintf(cache, "%" PRIuMAX, includes->hash[i]);
    }

    if (includes->size > 1) {
        fprintf(cache, "\n%*c", 2 * (depth - 1), ' ');
    }
    fprintf(cache, "]");
}

void serialize_key(FILE* cache, char* key, int depth) {
    fprintf(cache, "%*c\"%s\": ", 2 * depth, ' ', key);
}
