#include "os.h"

#if defined(__linux__)
    #include <dirent.h>
    #include <sys/stat.h>

    int os_dir_make(char* path) {
        return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    bld_os_dir* os_dir_open(char* path) {
        return (bld_os_dir*) opendir(path);
    }

    int os_dir_close(bld_os_dir* dir) {
        return closedir((DIR*) dir);
    }

    bld_os_file* os_dir_read(bld_os_dir* dir) {
        return readdir(dir);
    }

    char* os_file_name(bld_os_file* file) {
        return ((struct dirent*) file)->d_name;
    }

    uintmax_t os_file_id(bld_os_file* file) {
        return ((struct dirent*) file)->d_ino;
    }
#elif defined(_WIN32)
    #error "No support for windows yet"
#endif
