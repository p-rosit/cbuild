#include <ctype.h>
#include "../bld_core/iter.h"
#include "../bld_core/logging.h"
#include "handle.h"

typedef struct bld_handle_info {
    int current_arg;
    int current_expected;
    bld_array expected_index;
    int* positional_parsed;
} bld_handle_info;

bld_handle_info handle_info_new(bld_handle*);
void handle_info_free(bld_handle_info*);
bld_command_error handle_parse_required(bld_string*, bld_handle_info*, bld_handle*, bld_command*, bld_array*);
bld_command_error handle_parse_optional(bld_string*, bld_handle_info*, bld_handle*, bld_command*, bld_array*);
bld_command_error handle_parse_expected(bld_string*, bld_handle_info*, bld_handle*, bld_command*, bld_array*);
bld_command_error handle_parse_vargs(bld_string*, bld_handle_info*, bld_handle*, bld_command*, bld_array*);
bld_command_error handle_parse_flag(bld_string*, bld_handle_info*, bld_handle*, bld_command*, bld_array*);

bld_command command_new(bld_handle*);
void command_free_internal(bld_command*, bld_handle_info*);
bld_string command_error_at(bld_string*);
void handle_string_append_int(bld_string*, int);

bld_handle handle_new(char* name) {
    bld_handle handle;
    bld_string str;

    str = string_pack(name);
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

void handle_string_append_int(bld_string* str, int number) {
    char num[10 + BLD_COMMAND_MAX_ARGS_LEN];

    if (number < 0 || number > BLD_COMMAND_MAX_ARGS) {
        log_fatal("handle_string_append_int: number not in range [0, %d]", BLD_COMMAND_MAX_ARGS, number);
    }

    sprintf(num, "%d", number);
    string_append_string(str, num);
}

void handle_positional_required(bld_handle* handle, char* description) {
    bld_string str;
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

    str = string_pack(description);
    arg.type = BLD_HANDLE_POSITIONAL_REQUIRED;
    arg.description = string_copy(&str);

    array_push(&handle->positional, &arg);
}

void handle_positional_optional(bld_handle* handle, char* description) {
    bld_string str;
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

    str = string_pack(description);
    arg.type = BLD_HANDLE_POSITIONAL_OPTIONAL;
    arg.description = string_copy(&str);

    array_push(&handle->positional, &arg);
}

void handle_positional_vargs(bld_handle* handle, char* description) {
    bld_string str;
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

    str = string_pack(description);
    arg.type = BLD_HANDLE_POSITIONAL_VARGS;
    arg.description = string_copy(&str);

    array_push(&handle->positional, &arg);
}

void handle_positional_expect(bld_handle* handle, char* value) {
    bld_string val;
    bld_handle_positional arg;

    if (handle->positional.size >= BLD_COMMAND_MAX_ARGS) {
        log_fatal("handle_positional_required: cannot exceed %d arguments.", BLD_COMMAND_MAX_ARGS);
    }

    val = string_pack(value);
    arg.type = BLD_HANDLE_POSITIONAL_EXPECTED;
    arg.description = string_copy(&val);

    array_push(&handle->positional, &arg);
}

void handle_allow_arbitrary_flags(bld_handle* handle, char* description) {
    bld_string str;

    if (handle->arbitrary_flags) {
        log_fatal("handle_allow_arbitrary_flags: attempting to allow arbitrary flags multiple times");
    }

    str = string_pack(description);
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

    if (handle->description_set) {
        log_fatal("handle_add_description: Description of handle has already been set");
    }

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

bld_command command_new(bld_handle* handle) {
    bld_command cmd;
    bld_iter iter;
    bld_handle_positional* arg;

    cmd.positional = array_new(sizeof(bld_command_positional));
    cmd.flags = set_new(sizeof(bld_command_flag));
    cmd.extra_flags = array_new(sizeof(bld_command_flag));

    iter = iter_array(&handle->positional);
    while (iter_next(&iter, (void**) &arg)) {
        bld_command_positional cmd_arg;

        cmd_arg.type = arg->type;
        switch (cmd_arg.type) {
            case (BLD_HANDLE_POSITIONAL_REQUIRED):
                break;
            case (BLD_HANDLE_POSITIONAL_EXPECTED):
                cmd_arg.as.exp.value = string_copy(&arg->description);
                break;
            case (BLD_HANDLE_POSITIONAL_OPTIONAL):
                cmd_arg.as.opt.present = 0;
                break;
            case (BLD_HANDLE_POSITIONAL_VARGS):
                cmd_arg.as.vargs.values = array_new(sizeof(bld_string));
                break;
        }

        array_push(&cmd.positional, &cmd_arg);
    }

    return cmd;
}

void command_free(bld_command* cmd) {
    bld_iter iter;
    bld_command_flag* flag;
    bld_command_positional* cmd_arg;

    iter = iter_array(&cmd->positional);
    while (iter_next(&iter, (void**) &cmd_arg)) {
        bld_iter iter;
        bld_string* arg;

        switch (cmd_arg->type) {
            case (BLD_HANDLE_POSITIONAL_REQUIRED):
                string_free(&cmd_arg->as.req.value);
                break;
            case (BLD_HANDLE_POSITIONAL_EXPECTED):
                string_free(&cmd_arg->as.exp.value);
                break;
            case (BLD_HANDLE_POSITIONAL_OPTIONAL):
                if (cmd_arg->as.opt.present) {
                    string_free(&cmd_arg->as.opt.value);
                }
                break;
            case (BLD_HANDLE_POSITIONAL_VARGS):
                iter = iter_array(&cmd_arg->as.vargs.values);
                while (iter_next(&iter, (void**) &arg)) {
                    string_free(arg);
                }
                array_free(&cmd_arg->as.vargs.values);
                break;
        }
    }
    array_free(&cmd->positional);

    iter = iter_set(&cmd->flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(&flag->flag);
    }
    set_free(&cmd->flags);

    iter = iter_array(&cmd->extra_flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(&flag->flag);
    }
    array_free(&cmd->extra_flags);
}

void command_free_internal(bld_command* cmd, bld_handle_info* info) {
    size_t index;
    bld_iter iter;
    bld_command_positional* arg;
    bld_command_flag* flag;

    index = 0;
    iter = iter_array(&cmd->positional);
    while (iter_next(&iter, (void**) &arg)) {
        bld_iter iter;
        bld_string* str;

        switch (arg->type) {
            case (BLD_HANDLE_POSITIONAL_REQUIRED):
                if (info->positional_parsed[index]) {
                    string_free(&arg->as.req.value);
                }
                break;
            case (BLD_HANDLE_POSITIONAL_OPTIONAL):
                if (info->positional_parsed[index]) {
                    string_free(&arg->as.opt.value);
                }
                break;
            case (BLD_HANDLE_POSITIONAL_VARGS):
                iter = iter_array(&arg->as.vargs.values);
                while (iter_next(&iter, (void**) &str)) {
                    string_free(str);
                }
                array_free(&arg->as.vargs.values);
                break;
            case (BLD_HANDLE_POSITIONAL_EXPECTED):
                string_free(&arg->as.exp.value);
                break;
        }

        index += 1;
    }
    array_free(&cmd->positional);

    iter = iter_set(&cmd->flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(&flag->flag);
    }
    set_free(&cmd->flags);

    iter = iter_array(&cmd->extra_flags);
    while (iter_next(&iter, (void**) &flag)) {
        string_free(&flag->flag);
    }
    array_free(&cmd->extra_flags);
}

bld_string command_error_at(bld_string* arg) {
    bld_string err = string_new();
    
    string_append_string(&err, "Error at \"");
    string_append_string(&err, string_unpack(arg));
    string_append_string(&err, "\": ");

    return err;
}

bld_handle_info handle_info_new(bld_handle* handle) {
    bld_handle_info info;
    size_t index;
    bld_iter iter;
    bld_handle_positional* pos;

    info.positional_parsed = malloc(handle->positional.size * sizeof(int));
    if (info.positional_parsed == NULL) {
        log_fatal("handle_info_new: could not allocate parsed array of size %lu", handle->positional.size);
    }

    index = 0;
    info.expected_index = array_new(sizeof(size_t));
    iter = iter_array(&handle->positional);
    while (iter_next(&iter, (void**) &pos)) {
        if (pos->type == BLD_HANDLE_POSITIONAL_EXPECTED) {
            array_push(&info.expected_index, &index);
        }
        info.positional_parsed[index] = 0;
        index += 1;
    }

    info.current_arg = 0;
    info.current_expected = 0;
    return info;
}

void handle_info_free(bld_handle_info* info) {
    array_free(&info->expected_index);
    free(info->positional_parsed);
}

int handle_parse(bld_args args, bld_handle* handle, bld_command* cmd, bld_array* err) {
    int error, index, too_few;
    bld_string empty_switch, empty_option;
    bld_iter iter;
    bld_handle_info info;
    bld_handle_positional* pos;

    *err = array_new(sizeof(bld_string));
    *cmd = command_new(handle);
    info = handle_info_new(handle);

    error = 0;
    empty_switch = string_pack("-");
    empty_option = string_pack("--");
    while (!args_empty(&args)) {
        bld_string arg = args_advance(&args);

        if (*arg.chars != '-' || string_eq(&arg, &empty_switch) || string_eq(&arg, &empty_option)) {
            if (((size_t) info.current_arg) < handle->positional.size) {
                pos = array_get(&handle->positional, info.current_arg);
            } else {
                bld_string str = command_error_at(&arg);
                string_append_string(&str, "too many arguments");
                array_push(err, &str);
                error |= BLD_COMMAND_ERROR_ARGS_TOO_MANY;
                break;
            }

            switch (pos->type) {
                case (BLD_HANDLE_POSITIONAL_REQUIRED):
                    error |= handle_parse_required(&arg, &info, handle, cmd, err);
                    break;
                case (BLD_HANDLE_POSITIONAL_EXPECTED):
                    error |= handle_parse_expected(&arg, &info, handle, cmd, err);
                    break;
                case (BLD_HANDLE_POSITIONAL_OPTIONAL):
                    error |= handle_parse_optional(&arg, &info, handle, cmd, err);
                    break;
                case (BLD_HANDLE_POSITIONAL_VARGS):
                    error |= handle_parse_vargs(&arg, &info, handle, cmd, err);
                    break;
                default:
                    log_fatal("Unreachable");
            }

        } else if (handle->flag_start_index > info.current_arg) {
            bld_string str = command_error_at(&arg);
            string_append_string(&str, "flags can only be specified after argument ");
            handle_string_append_int(&str, handle->flag_start_index);
            array_push(err, &str);
            error |= BLD_COMMAND_ERROR_FLAG_EARLY;
        } else {
            error |= handle_parse_flag(&arg, &info, handle, cmd, err);
        }
    }


    index = -1;
    too_few = 0;
    iter = iter_array(&handle->positional);
    while (iter_next(&iter, (void**) &pos)) {
        index += 1;
        if (index < info.current_arg) {continue;}
        if (info.positional_parsed[index]) {continue;}

        if (!too_few && (pos->type == BLD_HANDLE_POSITIONAL_REQUIRED || pos->type == BLD_HANDLE_POSITIONAL_EXPECTED)) {
            bld_string str;
            str = string_new();
            string_append_string(&str, "Expected ");
            handle_string_append_int(&str, handle->positional.size);
            string_append_string(&str, " positional arguments to command, got ");
            handle_string_append_int(&str, info.current_arg);
            array_push(err, &str);
            error |= BLD_COMMAND_ERROR_ARGS_TOO_FEW;

            too_few = 1;
        }

        error |= BLD_COMMAND_ERROR_ARGS_NO_MATCH * (pos->type == BLD_HANDLE_POSITIONAL_EXPECTED);
    }

    if (error) {
        command_free_internal(cmd, &info);
    }
    handle_info_free(&info);
    
    return error;
}

bld_command_error handle_parse_required(bld_string* arg, bld_handle_info* info, bld_handle* handle, bld_command* cmd, bld_array* err) {
    bld_command_positional* cmd_arg;

    info->positional_parsed[info->current_arg] = 1;
    cmd_arg = array_get(&cmd->positional, info->current_arg);
    cmd_arg->as.req.value = string_copy(arg);

    info->current_arg += 1;
    (void)(handle);
    (void)(err);
    return 0;
}

bld_command_error handle_parse_optional(bld_string* arg, bld_handle_info* info, bld_handle* handle, bld_command* cmd, bld_array* err) {
    int error;
    size_t* index;
    bld_handle_positional* expected;
    bld_command_positional* cmd_arg;

    if (((size_t) info->current_expected) < info->expected_index.size) {
        index = array_get(&info->expected_index, info->current_expected);
        expected = array_get(&handle->positional, *index);
    } else {
        expected = NULL;
    }

    error = 0;
    if (expected != NULL) {
        if (string_eq(arg, &expected->description)) {
            info->current_arg = *index;
            error = handle_parse_expected(arg, info, handle, cmd, err);
        } else {
            info->positional_parsed[info->current_arg] = 1;
            cmd_arg = array_get(&cmd->positional, info->current_arg);
            cmd_arg->as.opt.present = 1;
            cmd_arg->as.opt.value = string_copy(arg);

            info->current_arg += 1;
        }
    } else {
        info->positional_parsed[info->current_arg] = 1;
        cmd_arg = array_get(&cmd->positional, info->current_arg);
        cmd_arg->as.opt.present = 1;
        cmd_arg->as.opt.value = string_copy(arg);
        info->current_arg += 1;
    }

    return error;
}

bld_command_error handle_parse_expected(bld_string* arg, bld_handle_info* info, bld_handle* handle, bld_command* cmd, bld_array* err) {
    int match;
    bld_handle_positional* expected;

    expected = array_get(&handle->positional, info->current_arg);
    match = string_eq(arg, &expected->description);

    info->positional_parsed[info->current_arg] = match;
    info->current_arg += 1;
    info->current_expected += 1;

    if (!match) {
        bld_string str;

        str = command_error_at(arg);
        string_append_string(&str, "expected \"");
        string_append_string(&str, string_unpack(&expected->description));
        string_append_string(&str, "\"");

        array_push(err, &str);
    }

    (void)(cmd);
    return !match * BLD_COMMAND_ERROR_ARGS_NO_MATCH;
}

bld_command_error handle_parse_vargs(bld_string* arg, bld_handle_info* info, bld_handle* handle, bld_command* cmd, bld_array* err) {
    int error = 0;
    size_t* index;
    bld_string str;
    bld_handle_positional* expected;
    bld_command_positional* cmd_arg;

    if (((size_t) info->current_expected) < info->expected_index.size) {
        index = array_get(&info->expected_index, info->current_expected);
        expected = array_get(&handle->positional, *index);
    } else {
        expected = NULL;
    }

    if (expected != NULL) {
        if (string_eq(arg, &expected->description)) {
            info->current_arg = *index;
            error = handle_parse_expected(arg, info, handle, cmd, err);
        } else {
            info->positional_parsed[info->current_arg] = 1;
            cmd_arg = array_get(&cmd->positional, info->current_arg);
            str = string_copy(arg);
            array_push(&cmd_arg->as.vargs.values, &str);
        }
    } else {
        info->positional_parsed[info->current_arg] = 1;
        cmd_arg = array_get(&cmd->positional, info->current_arg);
        str = string_copy(arg);
        array_push(&cmd_arg->as.vargs.values, &str);
    }

    return error;
}

bld_command_error handle_parse_flag(bld_string* arg, bld_handle_info* info, bld_handle* handle, bld_command* cmd, bld_array* err) {
    int flag_exists, is_switch;
    uintmax_t flag_hash;
    bld_string flag_str;
    bld_command_flag flag;
    char* temp;

    temp = string_unpack(arg) + 1;
    is_switch = *temp != '-';
    temp += *temp == '-';
    flag_str = string_pack(temp);

    if (*temp == '\0' && is_switch) {
        bld_string str;

        str = command_error_at(arg);
        string_append_string(&str, "empty switch");

        array_push(err, &str);
        return BLD_COMMAND_ERROR_FLAG_EMPTY;
    } else if (*temp == '\0' && !is_switch) {
        bld_string str;

        str = command_error_at(arg);
        string_append_string(&str, "empty flag");

        array_push(err, &str);
        return BLD_COMMAND_ERROR_FLAG_EMPTY;
    }    

    if (is_switch) {
        flag_hash = *temp;
        flag_exists = set_has(&handle->flags, *temp);
    } else {
        flag_hash = string_hash(temp);
        flag_exists = set_has(&handle->flags, string_hash(temp));
    }

    if (set_has(&cmd->flags, flag_hash)) {
        bld_string str;

        str = command_error_at(arg);
        string_append_string(&str, "duplicate flag");

        array_push(err, &str);
        return BLD_COMMAND_ERROR_FLAG_DUPLICATE;
    }

    if (flag_exists) {
        size_t* flag_index;
        bld_handle_flag* handle_flag;

        flag_index = set_get(&handle->flags, flag_hash);
        handle_flag = array_get(&handle->flag_array, *flag_index);

        flag.is_switch = is_switch;
        flag.flag = string_copy(&handle_flag->option);
        set_add(&cmd->flags, string_hash(string_unpack(&handle_flag->option)), &flag);
    } else if (handle->arbitrary_flags) {
        flag.is_switch = is_switch;
        flag.flag = string_copy(&flag_str);
        array_push(&cmd->extra_flags, &flag);
    } else {
        bld_string str;

        str = command_error_at(arg);
        if (is_switch) {
            string_append_string(&str, "unknown switch");
        } else {
            string_append_string(&str, "unknown option");
        }
        array_push(err, &str);
        return BLD_COMMAND_ERROR_FLAG_UNKNOWN;
    }

    (void)(info);
    return 0;
}

bld_string handle_make_description(bld_handle* handle) {
    int index, npos;
    bld_iter iter;
    bld_string description;
    bld_handle_positional* pos;
    bld_handle_flag* flag;

    description = string_new();
    string_append_string(&description, "usage: ");
    string_append_string(&description, string_unpack(&handle->name));

    npos = 0;
    index = 0;
    iter = iter_array(&handle->positional);
    while (iter_next(&iter, (void**) &pos)) {
        if (handle->flag_start_index >= 0 && handle->flag_start_index == index && (handle->flag_array.size > 0 || handle->arbitrary_flags)) {
            string_append_string(&description, " /");
        }
        string_append_space(&description);

        switch (pos->type) {
            case (BLD_HANDLE_POSITIONAL_REQUIRED):
                string_append_char(&description, '(');
                handle_string_append_int(&description, npos);
                string_append_char(&description, ')');
                break;
            case (BLD_HANDLE_POSITIONAL_VARGS):
                string_append_string(&description, "(...)");
                break;
            case (BLD_HANDLE_POSITIONAL_OPTIONAL):
                string_append_char(&description, '(');
                handle_string_append_int(&description, npos);
                string_append_char(&description, ')');
                break;
            case (BLD_HANDLE_POSITIONAL_EXPECTED):
                string_append_string(&description, string_unpack(&pos->description));
                break;
        }

        index += 1;
        npos += pos->type != BLD_HANDLE_POSITIONAL_EXPECTED && pos->type != BLD_HANDLE_POSITIONAL_VARGS;
    }

    if ((handle->flag_start_index < 0 || handle->flag_start_index == index) && (handle->flag_array.size > 0 || handle->arbitrary_flags)) {
        string_append_string(&description, " /");
    }

    iter = iter_array(&handle->flag_array);
    while (iter_next(&iter, (void**) &flag)) {
        string_append_space(&description);
        string_append_string(&description, "[-");
        if (flag->swtch != ' ') {
            string_append_char(&description, flag->swtch);
            string_append_string(&description, ", -");
        }
        string_append_char(&description, '-');
        string_append_string(&description, string_unpack(&flag->option));
        string_append_char(&description, ']');
    }

    if (handle->arbitrary_flags) {
        string_append_string(&description, " [arbitrary flags]");
    }

    string_append_string(&description, "\n\n");
    string_append_string(&description, string_unpack(&handle->description));
    if (handle->positional.size > 0) {
        string_append_string(&description, "\n\n");
        string_append_string(&description, "Positional Arguments:");
    }

    npos = 0;
    iter = iter_array(&handle->positional);
    while (iter_next(&iter, (void**) &pos)) {
        if (pos->type != BLD_HANDLE_POSITIONAL_EXPECTED) {

            string_append_string(&description, "\n  ");
            if (pos->type != BLD_HANDLE_POSITIONAL_VARGS) {
                handle_string_append_int(&description, npos);
            } else {
                string_append_string(&description, "...");
            }
            string_append_string(&description, ": ");
            if (pos->type == BLD_HANDLE_POSITIONAL_OPTIONAL) {
                string_append_string(&description, "[Optional] ");
            }
            string_append_string(&description, string_unpack(&pos->description));

            npos += pos->type != BLD_HANDLE_POSITIONAL_VARGS;
        }
    }

    if (handle->flag_array.size > 0 || handle->arbitrary_flags) {
        string_append_string(&description, "\n\n");
        string_append_string(&description, "Flags:");
    }

    iter = iter_array(&handle->flag_array);
    while (iter_next(&iter, (void**) &flag)) {
        string_append_char(&description, '\n');
        string_append_string(&description, "  -");
        if (flag->swtch != ' ') {
            string_append_char(&description, flag->swtch);
            string_append_string(&description, ", -");
        }
        string_append_char(&description, '-');
        string_append_string(&description, string_unpack(&flag->option));
        string_append_string(&description, " ");
        string_append_string(&description, string_unpack(&flag->description));
    }

    if (handle->arbitrary_flags) {
        string_append_string(&description, "\n  ");
        string_append_string(&description, string_unpack(&handle->arbitray_flags_description));
    }

    string_append_char(&description, '\n');
    return description;
}
