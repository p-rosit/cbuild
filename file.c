#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "os.h"
#include "logging.h"
#include "iter.h"
#include "linker.h"
#include "file.h"

bit_file_identifier get_identifier(bit_path*);
bit_file make_file(bit_file_type, bit_path*, char*);
bit_set file_copy_symbol_set(const bit_set*);
void file_free_base(bit_file*);
void file_free_dir(bit_file_dir*);
void file_free_impl(bit_file_impl*);
void file_free_header(bit_file_header*);
void file_free_test(bit_file_test*);

uintmax_t file_get_id(bit_path* path) {
    uintmax_t id = os_info_id(path_to_string(path));

    if (id == BIT_INVALID_IDENITIFIER) {
        log_fatal("Could not extract information about \"%s\"", path_to_string(path));
    }

    return id;
}

bit_file_identifier get_identifier(bit_path* path) {
    bit_file_identifier identifier;
    uintmax_t id, mtime;

    id = os_info_id(path_to_string(path));
    mtime = os_info_mtime(path_to_string(path));

    if (id == BIT_INVALID_IDENITIFIER) {
        log_fatal("Could not extract information about \"%s\"", path_to_string(path));
    }

    identifier.id = id;
    identifier.time = mtime; 
    identifier.hash = 0;

    return identifier;
}

int file_eq(bit_file* f1, bit_file* f2) {
    return f1->identifier.id == f2->identifier.id;
}

void serialize_identifier(char name[FILENAME_MAX], bit_file* file) {
    sprintf(name, "%" PRIuMAX, (uintmax_t) file->identifier.id);
}

bit_file make_file(bit_file_type type, bit_path* path, char* name) {
    bit_file file;
    bit_string str = string_pack(name);

    file.type = type;
    file.parent_id = BIT_INVALID_IDENITIFIER;
    file.identifier = get_identifier(path);
    file.name = string_copy(&str);
    file.path = *path;
    file.build_info.compiler_set = 0;
    file.build_info.linker_set = 0;

    return file;
}

bit_file file_dir_new(bit_path* path, char* name) {
    bit_file dir = make_file(BIT_DIR, path, name);
    dir.info.dir.files = array_new(sizeof(uintmax_t));
    return dir;
}

bit_file file_header_new(bit_path* path, char* name) {
    bit_file header = make_file(BIT_HEADER, path, name);
    header.info.header.includes = set_new(0);
    return header;
}

bit_file file_impl_new(bit_path* path, char* name) {
    bit_file impl = make_file(BIT_IMPL, path, name);
    impl.info.impl.includes = set_new(0);
    impl.info.impl.defined_symbols = set_new(sizeof(bit_string));
    impl.info.impl.undefined_symbols = set_new(sizeof(bit_string));
    return impl;
}

bit_file file_test_new(bit_path* path, char* name) {
    bit_file test = make_file(BIT_TEST, path, name);
    test.info.test.includes = set_new(0);
    test.info.test.undefined_symbols = set_new(sizeof(bit_string));
    return test;
}

void file_free(bit_file* file) {
    file_free_base(file);

    switch (file->type) {
        case (BIT_DIR): {
            file_free_dir(&file->info.dir);
        } break;
        case (BIT_IMPL): {
            file_free_impl(&file->info.impl);
        } break;
        case (BIT_HEADER): {
            file_free_header(&file->info.header);
        } break;
        case (BIT_TEST): {
            file_free_test(&file->info.test);
        } break;
        default: {log_fatal("file_free: unrecognized file type, unreachable error");}
    }
}

void file_free_base(bit_file* file) {
    if (file->build_info.compiler_set) {
        switch (file->build_info.compiler.type) {
            case (BIT_COMPILER): {
                compiler_free(&file->build_info.compiler.as.compiler);
            } break;
            case (BIT_COMPILER_FLAGS): {
                compiler_flags_free(&file->build_info.compiler.as.flags);
            } break;
            default: log_fatal("file_free_base: internal error");
        }
    }

    if (file->build_info.linker_set) {
        linker_flags_free(&file->build_info.linker_flags);
    }

    path_free(&file->path);
    string_free(&file->name);
}

void file_free_dir(bit_file_dir* dir) {
    array_free(&dir->files);
}

void file_free_impl(bit_file_impl* impl) {
    bit_iter iter;
    bit_string* symbol;

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

void file_free_header(bit_file_header* header) {
    set_free(&header->includes);
}

void file_free_test(bit_file_test* test) {
    bit_iter iter;
    bit_string* symbol;

    set_free(&test->includes);

    iter = iter_set(&test->undefined_symbols);
    while (iter_next(&iter, (void**) &symbol)) {
        string_free(symbol);
    }
    set_free(&test->undefined_symbols);
}

uintmax_t file_hash(bit_file* file, bit_set* files) {
    uintmax_t seed, parent_id;

    seed = 3401;
    seed = (seed << 3) + file->identifier.id;
    seed = (seed << 4) + seed + file->identifier.time;

    parent_id = file->identifier.id;
    while (parent_id != BIT_INVALID_IDENITIFIER) {
        bit_file* parent = set_get(files, parent_id);
        if (parent == NULL) {log_fatal("file_hash: internal error, hashing compiler");}
        parent_id = parent->parent_id;
        if (!parent->build_info.compiler_set) {continue;}

        switch (parent->build_info.compiler.type) {
            case (BIT_COMPILER): {
                seed = (seed << 3) + seed * compiler_hash(&parent->build_info.compiler.as.compiler);
            } break;
            case (BIT_COMPILER_FLAGS): {
                seed = (seed << 3) + seed * compiler_flags_hash(&parent->build_info.compiler.as.flags);
            } break;
        }

    }

    parent_id = file->identifier.id;
    while (parent_id != BIT_INVALID_IDENITIFIER) {
        bit_file* parent = set_get(files, parent_id);
        if (parent == NULL) {log_fatal("file_hash: internal error, hashing linker_flags");}
        parent_id = parent->parent_id;
        if (!parent->build_info.linker_set) {continue;}

        seed = (seed << 3) + seed * linker_flags_hash(&parent->build_info.linker_flags);
    }
    return seed;
}

void file_symbols_copy(bit_file* file, const bit_file* from) {
    if (file->type != from->type) {
        log_fatal("file_symbols_copy: files do not have same type");
    }

    switch (file->type) {
        case (BIT_IMPL): {
            file->info.impl.undefined_symbols = file_copy_symbol_set(&from->info.impl.undefined_symbols);
            file->info.impl.defined_symbols = file_copy_symbol_set(&from->info.impl.defined_symbols);
        } break;
        case (BIT_TEST): {
            file->info.test.undefined_symbols = file_copy_symbol_set(&from->info.test.undefined_symbols);
        } break;
        default: log_fatal("file_symbols_copy: file does not have any symbols to copy");
    }
}

bit_set file_copy_symbol_set(const bit_set* set) {
    bit_iter iter;
    bit_set cpy;
    bit_string str, *symbol;

    cpy = set_copy(set);
    iter = iter_set(&cpy);
    while (iter_next(&iter, (void**) &symbol)) {
        str = string_copy(symbol);
        *symbol = str;
    }

    return cpy;
}

void file_dir_add_file(bit_file* dir, bit_file* file) {
    if (dir->type != BIT_DIR) {log_fatal("file_dir_add_file: trying to add file, \"%s\", to non-directory, \"%s\"", string_unpack(&file->name), string_unpack(&dir->name));}

    file->parent_id = dir->identifier.id;
    array_push(&dir->info.dir.files, &file->identifier.id);
}

void file_assemble_compiler(bit_file* file, bit_set* files, bit_string** executable, bit_array* flags) {
    uintmax_t parent_id = file->identifier.id;
    *executable = NULL;
    *flags = array_new(sizeof(bit_compiler_flags));

    while (parent_id != BIT_INVALID_IDENITIFIER) {
        bit_file* parent;
        parent = set_get(files, parent_id);
        if (parent == NULL) {log_fatal("file_assemble_compiler: internal error");}
        parent_id = parent->parent_id;
        if (!parent->build_info.compiler_set) {continue;}

        if (parent->build_info.compiler.type == BIT_COMPILER) {
            *executable = &parent->build_info.compiler.as.compiler.executable;
            array_push(flags, &parent->build_info.compiler.as.compiler.flags);
            break;
        } else if (parent->build_info.compiler.type == BIT_COMPILER_FLAGS) {
            array_push(flags, &parent->build_info.compiler.as.flags);
        } else {
            log_fatal("file_assemble_compiler: internal error, compiler");
        }
    }

    array_reverse(flags);

    if (*executable == NULL) {
        log_fatal("file_assemble_compiler: no compiler was encountered while assembling compiler associated with file, only compiler flags. Root has no associated compiler");
    }
}

void file_assemble_linker_flags(bit_file* file, bit_set* files, bit_array* flags) {
    uintmax_t parent_id = file->identifier.id;
    *flags = array_new(sizeof(bit_linker_flags));

    while (parent_id != BIT_INVALID_IDENITIFIER) {
        bit_file* parent = set_get(files, parent_id);
        if (parent == NULL) {log_fatal("file_assemble_linker_flags: internal error");}
        parent_id = parent->parent_id;
        if (!parent->build_info.linker_set) {continue;}

        array_push(flags, &parent->build_info.linker_flags);
    }

    array_reverse(flags);
}
