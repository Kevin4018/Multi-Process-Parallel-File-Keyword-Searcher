#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "worker.h"
#include "protocol.h"
#include "util.h"

#define LINE_SIZE 1024

int count_keyword_in_file(const char *filename, const char *keyword) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        return -1;
    }

    char line[LINE_SIZE];
    int total_count = 0;
    size_t keyword_len = strlen(keyword);

    if (keyword_len == 0) {
        if (fclose(fp) != 0) {
            perror("fclose");
            return -1;
        }
        return 0;
    }

    while (fgets(line, LINE_SIZE, fp) != NULL) {
        char *pos = line;

        while ((pos = strstr(pos, keyword)) != NULL) {
            total_count++;
            pos += keyword_len;
        }
    }

    if (ferror(fp)) {
        perror("fgets");
        if (fclose(fp) != 0) {
            perror("fclose");
        }
        return -1;
    }

    if (fclose(fp) != 0) {
        perror("fclose");
        return -1;
    }

    return total_count;
}

void run_worker(int worker_id, int task_read_fd, int result_write_fd) {
    task_msg_t task;
    result_msg_t result;

    while (1) {
        ssize_t bytes_read = read_full(task_read_fd, &task, sizeof(task_msg_t));

        if (bytes_read == 0) {
            break;
        }

        if (bytes_read != (ssize_t)sizeof(task_msg_t)) {
            perror("worker read_full");
            break;
        }

        if (task.terminate == 1) {
            break;
        }

        memset(&result, 0, sizeof(result));
        result.job_id = task.job_id;
        result.worker_id = worker_id;

        int count = count_keyword_in_file(task.filename, task.keyword);
        if (count < 0) {
            result.match_count = 0;
            result.status = -1;
        } else {
            result.match_count = count;
            result.status = 0;
        }

        strncpy(result.filename, task.filename, MAX_FILENAME - 1);
        result.filename[MAX_FILENAME - 1] = '\0';

        ssize_t bytes_written = write_full(result_write_fd, &result, sizeof(result_msg_t));
        if (bytes_written != (ssize_t)sizeof(result_msg_t)) {
            perror("worker write_full");
            break;
        }
    }

    if (close(task_read_fd) < 0) {
        perror("close task_read_fd");
    }
    if (close(result_write_fd) < 0) {
        perror("close result_write_fd");
    }

    _exit(0);
}