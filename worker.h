#ifndef WORKER_H
#define WORKER_H

void run_worker(int worker_id, int task_read_fd, int result_write_fd);
int count_keyword_in_file(const char *filename, const char *keyword);

#endif