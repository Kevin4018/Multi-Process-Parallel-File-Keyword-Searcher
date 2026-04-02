#ifndef WORKER_H
#define WORKER_H

void run_worker(int worker_id, int task_read_fd, int result_write_fd);

#endif