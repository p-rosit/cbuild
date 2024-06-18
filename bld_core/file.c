#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "os.h"
#include "logging.h"
#include "iter.h"
#include "linker.h"
#include "file.h"

bld_file_identifier get_identifier(bld_path*);
bld_file make_file(bld_file_type, bld_path*, char*);
bld_set file_copy_symbol_set(const bld_set*);
void file_free_base(bld_file*);
void file_free_directory(bld_file_directory*);
void file_free_implementation(bld_file_implementation*);
void file_free_interface(bld_file_interface*);
void file_free_test(bld_file_test*);

uintmax_t file_get_id(bld_path* path) {
    uintmax_t id;

    id = os_info_id(path_to_string(path));
    if (id == BLD_INVALID_IDENITIFIER) {
        log_fatal(LOG_FATAL_PREFIX "could not extract information about \"%s\"", path_to_string(path));
    }

    return id;
}

bld_file_identifier get_identifier(bld_path* path) {
    bld_file_identifier identifier;
    uintmax_t id, mtime;

    id = os_info_id(path_to_string(path));
    mtime = os_info_mtime(path_to_string(path));

    if (id == BLD_INVALID_IDENITIFIER) {
        log_fatal(LOG_FATAL_PREFIX "could not extract information about \"%s\"", path_to_string(path));
    }

    identifier.id = id;
    identifier.time = mtime; 
    identifier.hash = 0;

    return identifier;
}

int file_eq(bld_file* f1, bld_file* f2) {
    return f1->identifier.id == f2->identifier.id;
}

void serialize_identifier(char name[FILENAME_MAX], bld_file* file) {
    sprintf(name, "%" PRIuMAX, (uintmax_t) file->identifier.id);
}

bld_file make_file(bld_file_type type, bld_path* path, char* name) {
    bld_file file;
    bld_string str;

    str = string_pack(name);

    file.type = type;
    file.parent_id = BLD_INVALID_IDENITIFIER;
    file.identifier = get_identifier(path);
    file.name = string_copy(&str);
    file.path = *path;
    file.build_info.compiler_set = 0;
    file.build_info.linker_set = 0;

    return file;
}

bld_file file_directory_new(bld_path* path, char* name) {
    bld_file dir;
    dir = make_file(BLD_FILE_DIRECTORY, path, name);
    dir.info.dir.files = array_new(sizeof(uintmax_t));
    return dir;
}

bld_file file_interface_new(bld_path* path, char* name) {
    bld_file header;
    header = make_file(BLD_FILE_INTERFACE, path, name);
    header.info.header.includes = set_new(0);
    return header;
}

bld_file file_implementation_new(bld_path* path, char* name) {
    bld_file impl;
    impl = make_file(BLD_FILE_IMPLEMENTATION, path, name);
    impl.info.impl.includes = set_new(0);
    impl.info.impl.defined_symbols = set_new(sizeof(bld_string));
    impl.info.impl.undefined_symbols = set_new(sizeof(bld_string));
    return impl;
}

bld_file file_test_new(bld_path* path, char* name) {
    bld_file test;
    test = make_file(BLD_FILE_TEST, path, name);
    test.info.test.includes = set_new(0);
    test.info.test.undefined_symbols = set_new(sizeof(bld_string));
    return test;
}

bld_set* file_includes_get(bld_file* file) {
    switch (file->type) {
        case (BLD_FILE_DIRECTORY):
            return NULL;
        case (BLD_FILE_IMPLEMENTATION):
            return &file->info.impl.includes;
        case (BLD_FILE_TEST):
            return &file->info.test.includes;
        case (BLD_FILE_INTERFACE):
            return &file->info.header.includes;
        case (BLD_FILE_INVALID):
            break;
    }

    log_fatal(LOG_FATAL_PREFIX "unrecognized file type %d", file->type);
    return NULL; /* unreachable */
}

bld_set* file_defined_get(bld_file* file) {
    switch (file->type) {
        case (BLD_FILE_DIRECTORY):
            return NULL;
        case (BLD_FILE_IMPLEMENTATION):
            return &file->info.impl.defined_symbols;
        case (BLD_FILE_TEST):
            return NULL;
        case (BLD_FILE_INTERFACE):
            return NULL;
        case (BLD_FILE_INVALID):
            break;
    }

    log_fatal(LOG_FATAL_PREFIX "unrecognized file type %d", file->type);
    return NULL; /* unreachable */
}

bld_set* file_undefined_get(bld_file* file) {
    switch (file->type) {
        case (BLD_FILE_DIRECTORY):
            return NULL;
        case (BLD_FILE_IMPLEMENTATION):
            return &file->info.impl.undefined_symbols;
        case (BLD_FILE_TEST):
            return &file->info.test.undefined_symbols;
        case (BLD_FILE_INTERFACE):
            return NULL;
        case (BLD_FILE_INVALID):
            break;
    }

    log_fatal(LOG_FATAL_PREFIX "unrecognized file type %d", file->type);
    return NULL; /* unreachable */
}

void file_free(bld_file* file) {
    file_free_base(file);

    switch (file->type) {
        case (BLD_FILE_DIRECTORY): {
            file_free_directory(&file->info.dir);
        } break;
        case (BLD_FILE_IMPLEMENTATION): {
            file_free_implementation(&file->info.impl);
        } break;
        case (BLD_FILE_INTERFACE): {
            file_free_interface(&file->info.header);
        } break;
        case (BLD_FILE_TEST): {
            file_free_test(&file->info.test);
        } break;
        default: {log_fatal(LOG_FATAL_PREFIX "unrecognized file type, unreachable error");}
    }
}

void file_free_base(bld_file* file) {
    file_build_info_free(&file->build_info);
    path_free(&file->path);
    string_free(&file->name);
}

void file_build_info_free(bld_file_build_information* info) {
    if (info->compiler_set) {
        switch (info->compiler.type) {
            case (BLD_COMPILER): {
                compiler_free(&info->compiler.as.compiler);
            } break;
            case (BLD_COMPILER_FLAGS): {
                compiler_flags_free(&info->compiler.as.flags);
            } break;
            default: log_fatal(LOG_FATAL_PREFIX "internal error");
        }
    }

    if (info->linker_set) {
        linker_flags_free(&info->linker_flags);
    }
}

void file_free_directory(bld_file_directory* dir) {
    array_free(&dir->files);
}

void file_free_implementation(bld_file_implementation* impl) {
    bld_iter iter;
    bld_string* symbol;

    set_free(&impl->includes);

    iter = iter_set(&impl->undefined_symbols);
    while (iter_next(&iter, (void**) &symbol)) {
        string_free(symbol);
    }
    set_free(&impl->undefined_symbols);

    iter = iter_set(&impl->defined_symbols);
    while (iter_next(&iter, (void**) &symbol)) {
        string_free(symbol);
    }
    set_free(&impl->defined_symbols);
}

void file_free_interface(bld_file_interface* header) {
    set_free(&header->includes);
}

void file_free_test(bld_file_test* test) {
    bld_iter iter;
    bld_string* symbol;

    set_free(&test->includes);

    iter = iter_set(&test->undefined_symbols);
    while (iter_next(&iter, (void**) &symbol)) {
        string_free(symbol);
    }
    set_free(&test->undefined_symbols);
}

uintmax_t file_hash(bld_file* file, bld_set* files) {
    uintmax_t seed, parent_id;

    seed = 3401;
    seed = (seed << 3) + file->identifier.id;
    seed = (seed << 4) + seed + file->identifier.time;

    parent_id = file->identifier.id;
    while (parent_id != BLD_INVALID_IDENITIFIER) {
        bld_file* parent;

        parent = set_get(files, parent_id);
        if (parent == NULL) {log_fatal(LOG_FATAL_PREFIX "internal error, hashing compiler");}
        parent_id = parent->parent_id;
        if (!parent->build_info.compiler_set) {continue;}

        switch (parent->build_info.compiler.type) {
            case (BLD_COMPILER): {
                seed = (seed << 3) + seed * compiler_hash(&parent->build_info.compiler.as.compiler);
            } break;
            case (BLD_COMPILER_FLAGS): {
                seed = (seed << 3) + seed * compiler_flags_hash(&parent->build_info.compiler.as.flags);
            } break;
        }

    }

    parent_id = file->identifier.id;
    while (parent_id != BLD_INVALID_IDENITIFIER) {
        bld_file* parent;

        parent = set_get(files, parent_id);
        if (parent == NULL) {log_fatal(LOG_FATAL_PREFIX "internal error, hashing linker_flags");}
        parent_id = parent->parent_id;
        if (!parent->build_info.linker_set) {continue;}

        seed = (seed << 3) + seed * linker_flags_hash(&parent->build_info.linker_flags);
    }
    return seed;
}

void file_symbols_copy(bld_file* file, bld_file* from) {
    if (file->type != from->type) {
        log_fatal(LOG_FATAL_PREFIX "files do not have same type");
    }

    {
        bld_set *undefined, *from_undefined;
        undefined = file_undefined_get(file);
        from_undefined = file_undefined_get(from);

        if ((undefined == NULL) != (from_undefined == NULL)) {
            log_fatal(LOG_FATAL_PREFIX "unreachable error");
        }
        if (undefined == NULL) {
            goto no_copy_undefined;
        }

        *undefined = file_copy_symbol_set(from_undefined);
    }
    no_copy_undefined:

    {
        bld_set *defined, *from_defined;
        defined = file_defined_get(file);
        from_defined = file_defined_get(from);

        if ((defined == NULL) != (from_defined == NULL)) {
            log_fatal(LOG_FATAL_PREFIX "unreachable error");
        }
        if (defined == NULL) {
            goto no_copy_defined;
        }

        *defined = file_copy_symbol_set(from_defined);
    }
    no_copy_defined:;
}

bld_set file_copy_symbol_set(const bld_set* set) {
    bld_iter iter;
    bld_set cpy;
    bld_string str, *symbol;

    cpy = set_copy(set);
    iter = iter_set(&cpy);
    while (iter_next(&iter, (void**) &symbol)) {
        str = string_copy(symbol);
        *symbol = str;
    }

    return cpy;
}

void file_dir_add_file(bld_file* dir, bld_file* file) {
    if (dir->type != BLD_FILE_DIRECTORY) {log_fatal(LOG_FATAL_PREFIX "trying to add file, \"%s\", to non-directory, \"%s\"", string_unpack(&file->name), string_unpack(&dir->name));}

    file->parent_id = dir->identifier.id;
    array_push(&dir->info.dir.files, &file->identifier.id);
}

void file_assemble_compiler(bld_file* file, bld_set* files, bld_compiler** compiler, bld_array* flags) {
    uintmax_t parent_id;

    parent_id = file->identifier.id;
    *compiler = NULL;
    *flags = array_new(sizeof(bld_compiler_flags));

    while (parent_id != BLD_INVALID_IDENITIFIER) {
        bld_file* parent;

        parent = set_get(files, parent_id);
        if (parent == NULL) {log_fatal(LOG_FATAL_PREFIX "internal error");}
        parent_id = parent->parent_id;
        if (!parent->build_info.compiler_set) {continue;}

        if (parent->build_info.compiler.type == BLD_COMPILER) {
            *compiler = &parent->build_info.compiler.as.compiler;
            array_push(flags, &parent->build_info.compiler.as.compiler.flags);
            break;
        } else if (parent->build_info.compiler.type == BLD_COMPILER_FLAGS) {
            array_push(flags, &parent->build_info.compiler.as.flags);
        } else {
            log_fatal(LOG_FATAL_PREFIX "internal error");
        }
    }

    array_reverse(flags);

    if (*compiler == NULL) {
        log_fatal(LOG_FATAL_PREFIX "no compiler was encountered while assembling compiler associated with file, only compiler flags. Root has no associated compiler");
    }
}

void file_assemble_linker_flags(bld_file* file, bld_set* files, bld_array* flags) {
    uintmax_t parent_id;

    parent_id = file->identifier.id;
    *flags = array_new(sizeof(bld_linker_flags));

    while (parent_id != BLD_INVALID_IDENITIFIER) {
        bld_file* parent;

        parent = set_get(files, parent_id);
        if (parent == NULL) {log_fatal(LOG_FATAL_PREFIX "internal error");}

        parent_id = parent->parent_id;
        if (!parent->build_info.linker_set) {continue;}

        array_push(flags, &parent->build_info.linker_flags);
    }

    array_reverse(flags);
}
