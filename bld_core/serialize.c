#include <inttypes.h>
#include "logging.h"
#include "project.h"
#include "json.h"

void serialize_rebuild_main(FILE*, bld_project*, int);
void serialize_files(FILE*, uintmax_t, bld_file*, bld_set*, int);
void serialize_file(FILE*, uintmax_t, bld_file*, bld_set*, int);
void serialize_file_type(FILE*, bld_file_type);
void serialize_file_id(FILE*, bld_file_identifier);
void serialize_file_mtime(FILE*, bld_file_identifier);
void serialize_file_symbols(FILE*, bld_set*, int);
void serialize_file_includes(FILE*, bld_set*, int);

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
    json_serialize_key(cache, "linker", depth);
    serialize_linker(cache, &project->base.linker, depth + 1);

    fprintf(cache, ",\n");
    json_serialize_key(cache, "files", depth);
    if (!project->base.rebuilding) {
        serialize_files(cache, BLD_INVALID_IDENITIFIER, root, &project->files, depth + 1);
    } else if (project->base.rebuilding) {
        serialize_files(cache, project->main_file, root, &project->files, depth + 1);

        fprintf(cache, ",\n");
        json_serialize_key(cache, "rebuild_main", depth);
        serialize_rebuild_main(cache, project, depth + 1);
    }

    fprintf(cache, "\n}\n");

    fclose(cache);
    path_free(&cache_path);
}

void serialize_rebuild_main(FILE* cache, bld_project* project, int depth) {
    bld_file* main;

    main = set_get(&project->files, project->main_file);
    if (main == NULL) {
        log_fatal(LOG_FATAL_PREFIX "internal error");
    }

    serialize_file(cache, project->main_file, main, &project->files, depth);
}

void serialize_files(FILE* cache, uintmax_t main, bld_file* root, bld_set* files, int depth) {
    serialize_file(cache, main, root, files, depth);
}

void serialize_file(FILE* cache, uintmax_t main, bld_file* file, bld_set* files, int depth) {
    fprintf(cache, "{\n");

    json_serialize_key(cache, "type", depth);
    serialize_file_type(cache, file->type);

    if (file->type != BLD_FILE_DIRECTORY) {
        fprintf(cache, ",\n");
        json_serialize_key(cache, "mtime", depth);
        serialize_file_mtime(cache, file->identifier);

        fprintf(cache, ",\n");
        json_serialize_key(cache, "hash", depth);
        fprintf(cache, "%" PRIuMAX, file->identifier.hash);
    }

    fprintf(cache, ",\n");
    json_serialize_key(cache, "name", depth);
    if (file->identifier.id != main) {
        fprintf(cache, "\"%s\"", string_unpack(&file->name));
    } else {
        fprintf(cache, "\"%s\"", path_to_string(&file->path));
    }

    if (file->build_info.compiler_set) {
        fprintf(cache, ",\n");
        switch (file->build_info.compiler.type) {
            case (BLD_COMPILER): {
                json_serialize_key(cache, "compiler", depth);
                serialize_compiler(cache, &file->build_info.compiler.as.compiler, depth + 1);
            } break;
            case (BLD_COMPILER_FLAGS): {
                json_serialize_key(cache, "compiler_flags", depth);
                serialize_compiler_flags(cache, &file->build_info.compiler.as.flags, depth + 1);
            } break;
        }
    }

    if (file->build_info.linker_set) {
        fprintf(cache, ",\n");
        json_serialize_key(cache, "linker_flags", depth);
        serialize_linker_flags(cache, &file->build_info.linker_flags, depth + 1);
    }

    {
        bld_set* includes;
        includes = file_includes_get(file);
        if (includes == NULL) {goto no_serialize_includes;}

        fprintf(cache, ",\n");
        json_serialize_key(cache, "includes", depth);
        serialize_file_includes(cache, includes, depth + 1);
    }
    no_serialize_includes:

    {
        bld_set* undefined;
        undefined = file_undefined_get(file);
        if (undefined == NULL) {goto no_serialize_undefined;}

        fprintf(cache, ",\n");

        json_serialize_key(cache, "undefined_symbols", depth);
        serialize_file_symbols(cache, undefined, depth + 1);
    }
    no_serialize_undefined:

    {
        bld_set* defined;
        defined = file_defined_get(file);
        if (defined == NULL) {goto no_serialize_defined;}

        fprintf(cache, ",\n");
        json_serialize_key(cache, "defined_symbols", depth);
        serialize_file_symbols(cache, defined, depth + 1);
    }
    no_serialize_defined:

    if (file->type == BLD_FILE_DIRECTORY) {
        int first = 1;
        bld_iter iter;
        uintmax_t* child_id;
        bld_file* child;

        fprintf(cache, ",\n");
        json_serialize_key(cache, "files", depth);
        fprintf(cache, "[");
        if (file->info.dir.files.size > 0) {
            fprintf(cache, "\n");
        }

        iter = iter_array(&file->info.dir.files);
        while (iter_next(&iter, (void**) &child_id)) {
            child = set_get(files, *child_id);
            if (child == NULL) {log_fatal("serialize_file: internal error");}
            if (child->identifier.id == main) {continue;}
            if ((child->type == BLD_FILE_IMPLEMENTATION || child->type == BLD_FILE_TEST) && !child->compile_successful) {continue;}

            if (!first) {
                fprintf(cache, ",\n");
            } else {
                first = 0;
            }
            fprintf(cache, "%*c", 2 * (depth + 1), ' ');
            serialize_file(cache, main, child, files, depth + 2);
        }

        if (file->info.dir.files.size > 0) {
            fprintf(cache, "\n%*c", 2 * depth, ' ');
        }
        fprintf(cache, "]");
    }

    fprintf(cache, "\n");
    fprintf(cache, "%*c}", 2 * (depth - 1), ' ');
}

void serialize_file_type(FILE* cache, bld_file_type type) {
    switch (type) {
        case (BLD_FILE_DIRECTORY): {
            fprintf(cache, "\"directory\"");
        } break;
        case (BLD_FILE_IMPLEMENTATION): {
            fprintf(cache, "\"implementation\"");
        } break;
        case (BLD_FILE_INTERFACE): {
            fprintf(cache, "\"interface\"");
        } break;
        case (BLD_FILE_TEST): {
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
    int first;
    bld_string* symbol;
    bld_iter iter;

    fprintf(cache, "[");
    if (symbols->size > 1) {
        fprintf(cache, "\n");
    }

    first = 1;
    iter = iter_set(symbols);
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
    int first;
    bld_iter iter;
    bld_path* path;

    fprintf(cache, "[");
    if (includes->size > 0) {
        fprintf(cache, "\n");
    }

    first = 1;
    iter = iter_set(includes);
    while (iter_next(&iter, (void**) &path)) {
        if (!first) {
            fprintf(cache, ",\n");
        } else {
            first = 0;
        }
        fprintf(cache, "%*c", 2 * depth, ' ');
        fprintf(cache, "\"%s\"", path_to_string(path));
    }

    if (includes->size > 0) {
        fprintf(cache, "\n%*c", 2 * (depth - 1), ' ');
    }
    fprintf(cache, "]");
}
