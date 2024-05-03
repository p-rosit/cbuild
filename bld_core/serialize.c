#include <stdio.h>
#include <inttypes.h>
#include "logging.h"
#include "project.h"

void serialize_compiler(FILE*, bld_compiler*, int);
void serialize_compiler_flags(FILE*, bld_compiler_flags*, int);
void serialize_compiler_flags_added_flags(FILE*, bld_compiler_flags*, int);
void serialize_compiler_flags_removed_flags(FILE*, bld_compiler_flags*, int);
void serialize_linker(FILE*, bld_linker*, int);
void serialize_linker_flags(FILE*, bld_linker_flags*, int);

void serialize_files(FILE*, bld_file*, bld_set*, int);
void serialize_file(FILE*, bld_file*, bld_set*, int);
void serialize_file_type(FILE*, bld_file_type);
void serialize_file_id(FILE*, bld_file_identifier);
void serialize_file_mtime(FILE*, bld_file_identifier);
void serialize_file_symbols(FILE*, bld_set*, int);
void serialize_file_includes(FILE*, bld_set*, int);

void serialize_key(FILE*, char*, int);

void project_save_cache(bld_project* project) {
    FILE* cache;
    bld_path cache_path;
    bld_file* root;
    int depth = 1;

    if (!project->base.cache.loaded) {
        log_fatal("Trying to save cache without a corresponding load cache, i.e. no cache path has been set.");
    }
    root = set_get(&project->files, project->root_dir);
    if (root == NULL) {log_fatal("project_save_cache: internal error");}

    cache_path = path_copy(&project->base.root);
    path_append_path(&cache_path, &project->base.cache.root);
    path_append_string(&cache_path, BLD_CACHE_NAME);

    cache = fopen(path_to_string(&cache_path), "w");
    if (cache == NULL) {
        log_fatal("Could not open cache file for writing under: \"%s\"", path_to_string(&cache_path));
    }

    fprintf(cache, "{\n");
    serialize_key(cache, "linker", depth);
    serialize_linker(cache, &project->base.linker, depth + 1);

    fprintf(cache, ",\n");
    serialize_key(cache, "files", depth);
    serialize_files(cache, root, &project->files, depth + 1);

    fprintf(cache, "\n}\n");

    fclose(cache);
    path_free(&cache_path);
}

void serialize_compiler(FILE* cache, bld_compiler* compiler, int depth) {
    fprintf(cache, "{\n");

    serialize_key(cache, "executable", depth);
    fprintf(cache, "\"%s\"", string_unpack(&compiler->executable));
    fprintf(cache, ",\n");

    serialize_key(cache, "flags", depth);
    serialize_compiler_flags(cache, &compiler->flags, depth + 1);

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_compiler_flags(FILE* cache, bld_compiler_flags* flags, int depth) {
    fprintf(cache, "{\n");

    serialize_key(cache, "added", depth);
    serialize_compiler_flags_added_flags(cache, flags, depth + 1);
    fprintf(cache, ",\n");

    serialize_key(cache, "removed", depth);
    serialize_compiler_flags_removed_flags(cache, flags, depth + 1);

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_compiler_flags_added_flags(FILE* cache, bld_compiler_flags* flags, int depth) {
    int first = 1;
    bld_iter iter;
    bld_string* flag;

    fprintf(cache, "[");
    if (flags->flags.size > 1) {
        fprintf(cache, "\n");
    }
    
    iter = iter_array(&flags->flags);
    while (iter_next(&iter, (void**) &flag)) {
        if (!first) {fprintf(cache, ",\n");}
        else {first = 0;}
        if (flags->flags.size > 1) {
            fprintf(cache, "%*c", 2 * depth, ' ');
        }
        fprintf(cache, "\"%s\"", string_unpack(flag));
    }

    if (flags->flags.size > 1) {
        fprintf(cache, "\n%*c", 2 * (depth - 1), ' ');
    }
    fprintf(cache, "]");
}

void serialize_compiler_flags_removed_flags(FILE* cache, bld_compiler_flags* flags, int depth) {
    int first = 1;
    bld_iter iter;
    bld_string* flag;

    fprintf(cache, "[");
    if (flags->removed.size > 1) {
        fprintf(cache, "\n");
    }

    iter = iter_set(&flags->removed);
    while (iter_next(&iter, (void**) &flag)) {
        if (!first) {fprintf(cache, ",\n");}
        else {first = 0;}
        if (flags->flags.size > 1) {
            fprintf(cache, "%*c", 2 * (depth + 1), ' ');
        }
        fprintf(cache, "\"%s\"", string_unpack(flag));
    }

    if (flags->removed.size > 1) {
        fprintf(cache, "\n%*c", 2 * depth, ' ');
    }
    fprintf(cache, "]");
}

void serialize_linker(FILE* cache, bld_linker* linker, int depth) {
    fprintf(cache, "{\n");

    serialize_key(cache, "executable", depth);
    fprintf(cache, "\"%s\"", string_unpack(&linker->executable));

    if (linker->flags.flags.size > 0) {
        fprintf(cache, ",\n");
        serialize_key(cache, "flags", depth);
        serialize_linker_flags(cache, &linker->flags, depth + 1);
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_linker_flags(FILE* cache, bld_linker_flags* flags, int depth) {
    bld_string* flag;
    int first = 1;
    bld_iter iter;

    fprintf(cache, "[");
    if (flags->flags.size > 1) {
        fprintf(cache, "\n");
    }
    
    iter = iter_array(&flags->flags);
    while (iter_next(&iter, (void**) &flag)) {
        if (!first) {fprintf(cache, ",\n");}
        else {first = 0;}
        if (flags->flags.size > 1) {
            fprintf(cache, "%*c", 2 * depth, ' ');
        }
        fprintf(cache, "\"%s\"", string_unpack(flag));
    }

    if (flags->flags.size > 1) {
        fprintf(cache, "\n%*c", 2 * (depth - 1), ' ');
    }
    fprintf(cache, "]");
}

void serialize_files(FILE* cache, bld_file* root, bld_set* files, int depth) {
    serialize_file(cache, root, files, depth);
}

void serialize_file(FILE* cache, bld_file* file, bld_set* files, int depth) {
    fprintf(cache, "{\n");

    serialize_key(cache, "type", depth);
    serialize_file_type(cache, file->type);

    fprintf(cache, ",\n");
    serialize_key(cache, "id", depth);
    serialize_file_id(cache, file->identifier);

    if (file->type != BLD_DIR) {
        fprintf(cache, ",\n");
        serialize_key(cache, "mtime", depth);
        serialize_file_mtime(cache, file->identifier);

        fprintf(cache, ",\n");
        serialize_key(cache, "hash", depth);
        fprintf(cache, "%" PRIuMAX, file->identifier.hash);
    }

    fprintf(cache, ",\n");
    serialize_key(cache, "name", depth);
    fprintf(cache, "\"%s\"", string_unpack(&file->name));

    if (file->build_info.compiler_set) {
        fprintf(cache, ",\n");
        switch (file->build_info.compiler.type) {
            case (BLD_COMPILER): {
                serialize_key(cache, "compiler", depth);
                serialize_compiler(cache, &file->build_info.compiler.as.compiler, depth + 1);
            } break;
            case (BLD_COMPILER_FLAGS): {
                serialize_key(cache, "compiler_flags", depth);
                serialize_compiler_flags(cache, &file->build_info.compiler.as.flags, depth + 1);
            } break;
        }
    }

    if (file->build_info.linker_set) {
        fprintf(cache, ",\n");
        serialize_key(cache, "linker_flags", depth);
        serialize_linker_flags(cache, &file->build_info.linker_flags, depth + 1);
    }

    if (file->type != BLD_DIR) {
        bld_set* includes;
        switch (file->type) {
            case (BLD_HEADER): {
                includes = &file->info.header.includes;
            } break;
            case (BLD_IMPL): {
                includes = &file->info.impl.includes;
            } break;
            case (BLD_TEST): {
                includes = &file->info.test.includes;
            } break;
            default: {
                log_fatal("serialize_file: unrecognized file type, unreachable error");
                return; /* Unreachable */
            }
        }

        fprintf(cache, ",\n");
        serialize_key(cache, "includes", depth);
        serialize_file_includes(cache, includes, depth + 1);
    }

    switch (file->type) {
        case (BLD_DIR): break;
        case (BLD_HEADER): break;
        case (BLD_IMPL): {
            fprintf(cache, ",\n");

            serialize_key(cache, "undefined_symbols", depth);
            serialize_file_symbols(cache, &file->info.impl.undefined_symbols, depth + 1);

            fprintf(cache, ",\n");
            serialize_key(cache, "defined_symbols", depth);
            serialize_file_symbols(cache, &file->info.impl.defined_symbols, depth + 1);

        } break;
        case (BLD_TEST): {
            fprintf(cache, ",\n");
            serialize_key(cache, "undefined_symbols", depth);
            serialize_file_symbols(cache, &file->info.test.undefined_symbols, depth + 1);
        } break;
        default: log_fatal("serialize_file: unrecognized file type, unreachable error");
    }

    if (file->type == BLD_DIR) {
        int first = 1;
        bld_iter iter;
        uintmax_t* child_id;
        bld_file* child;

        fprintf(cache, ",\n");
        serialize_key(cache, "files", depth);
        fprintf(cache, "[\n");

        iter = iter_array(&file->info.dir.files);
        while (iter_next(&iter, (void**) &child_id)) {
            child = set_get(files, *child_id);
            if (child == NULL) {log_fatal("serialize_file: internal error");}

            if (!first) {
                fprintf(cache, ",\n");
            } else {
                first = 0;
            }
            fprintf(cache, "%*c", 2 * (depth + 1), ' ');
            serialize_file(cache, child, files, depth + 2);
        }

        fprintf(cache, "\n%*c", 2 * depth, ' ');
        fprintf(cache, "]");
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_file_type(FILE* cache, bld_file_type type) {
    switch (type) {
        case (BLD_DIR): {
            fprintf(cache, "\"directory\"");
        } break;
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
    bld_string* symbol;
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
        fprintf(cache, "\"%s\"", string_unpack(symbol));
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