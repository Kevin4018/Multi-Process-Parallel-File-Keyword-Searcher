#ifndef UTIL_H
#define UTIL_H

#include <unistd.h>
#include <stddef.h>

ssize_t write_full(int fd, const void *buf, size_t count);
ssize_t read_full(int fd, void *buf, size_t count);

#endif