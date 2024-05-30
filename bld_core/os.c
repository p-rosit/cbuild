#include <stdio.h>
#include "logging.h"
#include "os.h"

int os_file_exists(char* path) {
    FILE* file = fopen(path, "r");
    if (file != NULL) {
        fclose(file);
    }
    return file != NULL;
}

#if defined(__linux__)
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>

    int os_cwd(char* buffer, int length) {
        if (length <= 0) {log_fatal("os_cwd: negative buffer length");}
        return getcwd(buffer, length) != NULL;
    }

    int os_dir_exists(char* path) {
        bld_os_dir* dir = os_dir_open(path);
        if (dir != NULL) {
            os_dir_close(dir);
        }
        return dir == NULL;
    }

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

    uintmax_t os_info_id(char* path) {
        struct stat file;
        if (stat(path, &file) < 0) {
            return BLD_INVALID_IDENITIFIER;
        }
        return file.st_ino;
    }

    uintmax_t os_info_mtime(char* path) {
        struct stat file;
        if (stat(path, &file) < 0) {
            return 0;
        }
        return file.st_mtime;
    }
#elif defined(_WIN32)
    #error "No support for windows yet"
#else
    #error "Unknown environment"
#endif
