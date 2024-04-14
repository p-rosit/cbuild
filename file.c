#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "os.h"
#include "logging.h"
#include "iter.h"
#include "file.h"

bld_file_identifier get_identifier(bld_path*);
bld_file make_file(bld_file_type, bld_path*, char*);
bld_set file_copy_symbol_set(const bld_set*);
void file_free_base(bld_file*);
void file_free_dir(bld_file_dir*);
void file_free_impl(bld_file_impl*);
void file_free_header(bld_file_header*);
void file_free_test(bld_file_test*);

uintmax_t file_get_id(bld_path* path) {
    uintmax_t id = os_info_id(path_to_string(path));

    if (id == BLD_INVALID_IDENITIFIER) {
        log_fatal("Could not extract information about \"%s\"", path_to_string(path));
    }

    return id;
}

bld_file_identifier get_identifier(bld_path* path) {
    bld_file_identifier identifier;
    uintmax_t id, mtime;

    id = os_info_id(path_to_string(path));
    mtime = os_info_mtime(path_to_string(path));

    if (id == BLD_INVALID_IDENITIFIER) {
        log_fatal("Could not extract information about \"%s\"", path_to_string(path));
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
    bld_string str = string_new();
    string_append_string(&str, name);

    file.type = type;
    file.identifier = get_identifier(path);
    file.name = str;
    file.path = *path;
    file.compiler = -1;
    file.linker_flags = -1;
    file.includes = set_new(0);

    return file;
}

bld_file file_dir_new(bld_path* path, char* name) {
    bld_file dir = make_file(BLD_DIR, path, name);
    return dir;
}

bld_file file_header_new(bld_path* path, char* name) {
    bld_file header = make_file(BLD_HEADER, path, name);
    return header;
}

bld_file file_impl_new(bld_path* path, char* name) {
    bld_file impl = make_file(BLD_IMPL, path, name);
    impl.info.impl.defined_symbols = set_new(sizeof(bld_string));
    impl.info.impl.undefined_symbols = set_new(sizeof(bld_string));
    return impl;
}

bld_file file_test_new(bld_path* path, char* name) {
    bld_file test = make_file(BLD_TEST, path, name);
    test.info.test.undefined_symbols = set_new(sizeof(bld_string));
    return test;
}

void file_free(bld_file* file) {
    file_free_base(file);

    switch (file->type) {
        case (BLD_IMPL): {
            file_free_impl(&file->info.impl);
        } break;
        case (BLD_HEADER): {
            file_free_header(&file->info.header);
        } break;
        case (BLD_TEST): {
            file_free_test(&file->info.test);
        } break;
        default: {log_fatal("file_free: unrecognized file type, unreachable error");}
    }
}

void file_free_base(bld_file* file) {
    path_free(&file->path);
    string_free(&file->name);
    set_free(&file->includes);
}

void file_free_dir(bld_file_dir* dir) {
    (void)(dir);
}

void file_free_impl(bld_file_impl* impl) {
    bld_iter iter;
    bld_string* symbol;

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

void file_free_header(bld_file_header* header) {
    (void)(header);
}

void file_free_test(bld_file_test* test) {
    bld_iter iter;
    bld_string* symbol;

    iter = iter_set(&test->undefined_symbols);
    while (iter_next(&iter, (void**) &symbol)) {
        string_free(symbol);
    }
    set_free(&test->undefined_symbols);
}

uintmax_t file_hash(bld_file* file, bld_array* compilers, uintmax_t seed) {
    seed = (seed << 3) + file->identifier.id;
    seed = (seed << 4) + seed + file->identifier.time;
    if (file->compiler > 0) {
        seed = compiler_hash(array_get(compilers, file->compiler), seed);
    }
    return seed;
}

void file_symbols_copy(bld_file* file, const bld_file* from) {
    if (file->type != from->type) {
        log_fatal("file_symbols_copy: files do not have same type");
    }

    switch (file->type) {
        case (BLD_IMPL): {
            file->info.impl.undefined_symbols = file_copy_symbol_set(&from->info.impl.undefined_symbols);
            file->info.impl.defined_symbols = file_copy_symbol_set(&from->info.impl.defined_symbols);
        } break;
        case (BLD_TEST): {
            file->info.test.undefined_symbols = file_copy_symbol_set(&from->info.test.undefined_symbols);
        } break;
        default: log_fatal("file_symbols_copy: file does not have any symbols to copy");
    }
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
