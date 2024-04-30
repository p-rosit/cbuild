#include <stdio.h>
#include "logging.h"
#include "os.h"

int os_file_exists(char* path) {
    FILE* file = fopen(path, "r");

    if (file == NULL) {
        return 0;
    } else {
        fclose(file);
        return 1;
    }
}

#if defined(__linux__)
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>

    int os_cwd(char* buffer, int length) {
        if (length <= 0) {log_fatal("os_cwd: negative buffer length");}
        return getcwd(buffer, length) != NULL;
    }

    int os_dir_make(char* path) {
        return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    bit_os_dir* os_dir_open(char* path) {
        return (bit_os_dir*) opendir(path);
    }

    int os_dir_close(bit_os_dir* dir) {
        return closedir((DIR*) dir);
    }

    bit_os_file* os_dir_read(bit_os_dir* dir) {
        return readdir(dir);
    }

    char* os_file_name(bit_os_file* file) {
        return ((struct dirent*) file)->d_name;
    }

    uintmax_t os_file_id(bit_os_file* file) {
        return ((struct dirent*) file)->d_ino;
    }

    uintmax_t os_info_id(char* path) {
        struct stat file;
        if (stat(path, &file) < 0) {
            return BIT_INVALID_IDENITIFIER;
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
#endif
