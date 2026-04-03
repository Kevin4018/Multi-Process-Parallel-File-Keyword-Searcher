#include "util.h"
#include <errno.h>

ssize_t read_full(int fd, void *buf, size_t count) {
    size_t total_read = 0;
    char *ptr = (char *)buf;

    while (total_read < count) {
        ssize_t bytes_read = read(fd, ptr + total_read, count - total_read);
        // fd[1] == 0
        if (bytes_read == 0) {
            return total_read; 
        }
        // error
        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        
        total_read += bytes_read;
    }
    return total_read;
}


ssize_t write_full(int fd, const void *buf, size_t count) {
    size_t total_written = 0;
    const char *ptr = (const char *)buf;
    while (total_written < count) {
        ssize_t bytes_written = write(fd, ptr + total_written, count - total_written);
        //fd[1] == 0
        if (bytes_written == 0) {
            return total_written;
        }
        // error
        if (bytes_written == -1) {
            if (errno == EINTR) {
                continue; 
            }
            return -1; 
        }
        
        total_written += bytes_written;
    }
    return total_written;
}