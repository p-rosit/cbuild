#include <ctype.h>
#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "handle.h"

bld_handle handle_new(char* name) {
    bld_handle handle;
    bld_string str = string_pack(name);
    handle.name = string_copy(&str);
    handle.description_set = 0;
    handle.positional = array_new(sizeof(bld_handle_positional));
    handle.arbitrary_flags = 0;
    handle.flag_start_index = -1;
    handle.flag_array = array_new(sizeof(bld_handle_flag));
    handle.flags = set_new(sizeof(size_t));
    return handle;
}

void handle_free(bld_handle* handle) {
    bld_iter iter;
    bld_handle_positional* arg;
    bld_handle_flag* flag;

    string_free(&handle->name);
    if (handle->description_set) {
        string_free(&handle->description);
    }
    if (handle->arbitrary_flags) {
        string_free(&handle->arbitray_flags_description);
    }

    iter = iter_array(&handle->positional);
    while (iter_next(&iter, (void**) &arg)) {
        string_free(&arg->description);
    }
    array_free(&handle->positional);

    iter = iter_array(&handle->flag_array);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(&flag->option);
        string_free(&flag->description);
    }
    array_free(&handle->flag_array);
    set_free(&handle->flags);
}

void handle_positional_required(bld_handle* handle, char* description) {
    bld_string str = string_pack(description);
    bld_handle_positional arg, *prev;
    
    if (handle->positional.size >= BLD_COMMAND_MAX_ARGS) {
        log_fatal("handle_positional_required: cannot exceed %d arguments.", BLD_COMMAND_MAX_ARGS);
    }

    if (handle->positional.size > 0) {
        prev = array_get(&handle->positional, handle->positional.size - 1);
        if (prev->type == BLD_HANDLE_POSITIONAL_OPTIONAL) {
            log_fatal("handle_positional_required: attempting to add required argument after optional argument");
        } else if (prev->type == BLD_HANDLE_POSITIONAL_VARGS) {
            log_fatal("handle_positional_required: attempting to add required argument after vargs");
        }
    }

    arg.type = BLD_HANDLE_POSITIONAL_REQUIRED;
    arg.description = string_copy(&str);
    array_push(&handle->positional, &arg);
}

void handle_positional_optional(bld_handle* handle, char* description) {
    bld_string str = string_pack(description);
    bld_handle_positional arg, *prev;
    
    if (handle->positional.size >= BLD_COMMAND_MAX_ARGS) {
        log_fatal("handle_positional_required: cannot exceed %d arguments.", BLD_COMMAND_MAX_ARGS);
    }

    if (handle->positional.size > 0) {
        prev = array_get(&handle->positional, handle->positional.size - 1);
        if (prev->type == BLD_HANDLE_POSITIONAL_VARGS) {
            log_fatal("handle_positional_optional: attempting to add optional argument after vargs");
        }
    }


    arg.type = BLD_HANDLE_POSITIONAL_OPTIONAL;
    arg.description = string_copy(&str);
    array_push(&handle->positional, &arg);
}

void handle_positional_vargs(bld_handle* handle, char* description) {
    bld_string str = string_pack(description);
    bld_handle_positional arg, *prev;
    
    if (handle->positional.size >= BLD_COMMAND_MAX_ARGS) {
        log_fatal("handle_positional_required: cannot exceed %d arguments.", BLD_COMMAND_MAX_ARGS);
    }

    if (handle->positional.size > 0) {
        prev = array_get(&handle->positional, handle->positional.size - 1);
        if (prev->type == BLD_HANDLE_POSITIONAL_VARGS) {
            log_fatal("handle_positional_vargs: attempting to add vargs after vargs");
        } else if (prev->type == BLD_HANDLE_POSITIONAL_OPTIONAL) {
            log_fatal("handle_positional_vargs: attempting to add vargs after optional argument");
        }
    }

    arg.type = BLD_HANDLE_POSITIONAL_VARGS;
    arg.description = string_copy(&str);

    array_push(&handle->positional, &arg);
}

void handle_positional_expect(bld_handle* handle, char* value) {
    bld_string val = string_pack(value);
    bld_handle_positional arg;

    if (handle->positional.size >= BLD_COMMAND_MAX_ARGS) {
        log_fatal("handle_positional_required: cannot exceed %d arguments.", BLD_COMMAND_MAX_ARGS);
    }

    arg.type = BLD_HANDLE_POSITIONAL_EXPECTED;
    arg.description = string_copy(&val);
    array_push(&handle->positional, &arg);
}

void handle_allow_arbitrary_flags(bld_handle* handle, char* description) {
    bld_string str = string_pack(description);
    if (handle->arbitrary_flags) {
        log_fatal("handle_allow_arbitrary_flags: attempting to allow arbitrary flags multiple times");
    }

    handle->arbitrary_flags = 1;
    handle->arbitray_flags_description = string_copy(&str);
}

void handle_allow_flags(bld_handle* handle) {
    bld_handle_positional* pos;

    if (handle->flag_start_index > 0) {
        log_fatal("handle_allow_flags: Flag start index has already been set");
    }

    if (handle->positional.size == 0) {
        pos = NULL;
    } else {
        pos = array_get(&handle->positional, handle->positional.size - 1);
    }

    if (pos == NULL) {
        handle->flag_start_index = handle->positional.size;
    } else if (pos->type != BLD_HANDLE_POSITIONAL_VARGS) {
        handle->flag_start_index = handle->positional.size;
    } else {
        log_fatal("handle_allow_flags: attempting to allow flags after variable arguments");
    }
}

void handle_set_description(bld_handle* handle, char* description) {
    bld_string str;
    str = string_pack(description);
    handle->description_set = 1;
    handle->description = string_copy(&str);
}

void handle_flag(bld_handle* handle, char swtch, char* option, char* description) {
    size_t index = handle->flag_array.size;
    bld_string opt, desc;
    bld_handle_flag flag;
    opt = string_pack(option);
    desc = string_pack(description);

    flag.description = string_copy(&desc);
    flag.swtch = swtch;
    flag.option = string_copy(&opt);

    array_push(&handle->flag_array, &flag);

    if (set_has(&handle->flags, swtch)) {
        log_fatal("handle_add_flag: switch '%c' added multiple times", swtch);
    } else if (set_has(&handle->flags, string_hash(option))) {
        log_fatal("handle_add_flag: option \"%s\" added multiple times", option);
    }

    if (isalpha(swtch)) {
        set_add(&handle->flags, swtch, &index);
    } else if (swtch != ' ') {
        log_fatal("handle_add_flag: invalid switch '%c', valid are [A-Za-z] and ' ' is ignored", swtch);
    }

    set_add(&handle->flags, string_hash(option), &index);
}
