#ifndef OS_H
#define OS_H
#include <inttypes.h>

#define BLD_INVALID_IDENITIFIER (0)

typedef void bld_os_dir;
typedef void bld_os_file;

int             os_cwd(char*, int);

int             os_dir_make(char*);
bld_os_dir*     os_dir_open(char*);
int             os_dir_close(bld_os_dir*);
bld_os_file*    os_dir_read(bld_os_dir*);

int             os_file_exists(char*);
char*           os_file_name(bld_os_file*);
uintmax_t       os_file_id(bld_os_file*);

uintmax_t       os_info_id(char*);
uintmax_t       os_info_mtime(char*);

#endif
