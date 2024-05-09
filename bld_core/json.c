#include <ctype.h>
#include <string.h>
#include "logging.h"
#include "dstr.h"
#include "json.h"

void json_serialize_key(FILE* cache, char* key, int depth) {
    fprintf(cache, "%*c\"%s\": ", 2 * depth, ' ', key);
}

int json_parse_array(FILE* file, void* obj, bld_parse_func parse_func) {
    int value_num = 0;
    int result, parse_complete;
    int c;

    parse_complete = 0;
    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c != '[') {log_warn("Unexpected starting character: \'%c\'", c); goto parse_failed;}

    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c == ']') {
        parse_complete = 1;
    } else {
        ungetc(c, file);
    }

    while (!parse_complete) {
        value_num += 1;
        result = parse_func(file, obj);
        if (result) {goto parse_failed;}
        
        c = next_character(file);
        switch (c) {
            case (']'): {
                parse_complete = 1;
            } break;
            case (','): {
                continue;
            } break;
            case (EOF): {
                log_warn("Unexpected EOF");
                goto parse_failed;
            } break;
            default: {
                log_warn("Unexpected character, got \'%c\'", c);
                goto parse_failed;
            } break;
        }
    }

    return value_num;
    parse_failed:
    return -1;
}

int json_parse_map(FILE* file, void* obj, int entries, int* parsed, char** keys, bld_parse_func* parse_funcs) {
    int exists, index, key_num = 0;
    int result, parse_complete;
    bld_string str;
    char c, *temp;

    memset(parsed, 0, entries * sizeof(int));

    parse_complete = 0;
    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c != '{') {log_warn("Unexpected starting character: \'%c\'", c); goto parse_failed;}

    c = next_character(file);
    if (c == EOF) {log_warn("Unexpected EOF"); goto parse_failed;}
    if (c == '}') {
        parse_complete = 1;
    } else {
        ungetc(c, file);
    }

    while (!parse_complete) {
        int i;

        key_num += 1;
        result = string_parse(file, &str);
        if (result) {
            log_warn("Key %d could not be parsed, expected: [", key_num);
            for (i = 0; i < entries; i++) {
                if (i > 0) {printf(",\n");}
                printf("  \"%s\"", keys[i]);
            }
            printf("\n]\n");
            goto parse_key_failed;
        }

        exists = 0;
        temp = string_unpack(&str);
        for (i = 0; i < entries; i++) {
            if (strcmp(temp, keys[i]) == 0) {
                exists = 1;
                index = i;
            }
        }

        if (!exists) {
            log_warn("\"%s\" is not a valid key, expected: [", temp);
            for (i = 0; i < entries; i++) {
                if (i > 0) {printf(",\n");}
                printf("  \"%s\"", keys[i]);
            }
            printf("\n]\n");
            goto parse_value_failed;
        }
        if (parsed[index]) {
            log_warn("Duplicate key \"%s\" encountered", keys[index]);
            goto parse_value_failed;
        }

        c = next_character(file);
        if (c != ':') {
            log_warn("Expected \':\', got \'%c\'", c);
            goto parse_value_failed;
        }

        result = parse_funcs[index](file, obj);
        if (result) {goto parse_value_failed;}
        parsed[index] = 1;

        c = next_character(file);
        switch (c) {
            case ('}'): {
                string_free(&str);
                parse_complete = 1;
            } break;
            case (','): {
                string_free(&str);
                continue;
            } break;
            case (EOF): {
                log_warn("Unexpected EOF");
                goto parse_value_failed;
            } break;
            default: {
                log_warn("Unexpected character, got \'%c\'", c);
                goto parse_value_failed;
            } break;
        }
    }

    return key_num;
    parse_value_failed:
    string_free(&str);
    parse_key_failed:
    parse_failed:
    return -1;
}

int parse_uintmax(FILE* file, uintmax_t* num_ptr) {
    uintmax_t num = 0;
    int c;

    c = next_character(file);
    if (!isdigit(c)) {
        log_warn("Expected number, got: \'%c\'", c);
        return -1;
    }

    while (c != EOF && isdigit(c)) {
        /* Warning: number is assumed to be valid, no overflow etc. */
        num = 10 * num + (c - '0');
        c = getc(file);
    }
    ungetc(c, file);

    *num_ptr = num;
    return 0;
}

int next_character(FILE* file) {
    int c = getc(file);
    while (c != EOF && isspace(c)) {c = getc(file);}
    return c;
}
