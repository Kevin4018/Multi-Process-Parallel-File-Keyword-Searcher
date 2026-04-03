#include <unistd.h>
#include <errno.h>

ssize_t write_full(int fd, const void *buf, size_t count) {
    size_t total_written = 0;
    const char *ptr = buf;

    while (total_written < count) {
        ssize_t n = write(fd, ptr + total_written, count - total_written);
        // add another check if the write returns 0, then quit immediately 
        if (n == 0){
            return total_written;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_written += n;
    }

    return total_written;
}

ssize_t read_full(int fd, void *buf, size_t count) {
    size_t total_read = 0;
    char *ptr = buf;

    while (total_read < count) {
        ssize_t n = read(fd, ptr + total_read, count - total_read);
        /* add another chekc if the pipe is closed, then read will return 0
           at this time, fd[1] == 0, so the writting ends*/
        if (n == 0) {
            return total_read;
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) {
            break;
        }
        total_read += n;
    }

    return total_read;
}