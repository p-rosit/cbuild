#ifndef OS_H
#define OS_H
#include <inttypes.h>

#define BIT_INVALID_IDENITIFIER (0)

typedef void bit_os_dir;
typedef void bit_os_file;

int             os_cwd(char*, int);

int             os_dir_make(char*);
bit_os_dir*     os_dir_open(char*);
int             os_dir_close(bit_os_dir*);
bit_os_file*    os_dir_read(bit_os_dir*);

int             os_file_exists(char*);
char*           os_file_name(bit_os_file*);
uintmax_t       os_file_id(bit_os_file*);

uintmax_t       os_info_id(char*);
uintmax_t       os_info_mtime(char*);

#endif
