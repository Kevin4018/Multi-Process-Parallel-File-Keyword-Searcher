#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "protocol.h"
#include "worker.h"
#include "util.h"

#define NUM_WORKERS 3

typedef struct {
    int task_write_fd;     // parent -> worker
    int result_read_fd;    // worker -> parent
    pid_t pid;
    int worker_id;
    int busy;              // 0 = idle, 1 = processing a job
    int alive;             // 1 = alive, 0 = dead/unusable
} worker_info_t;

static void usage(const char *progname);
static int spawn_worker(worker_info_t *worker);
static void close_worker_parent_fds(worker_info_t *worker);
static int send_task(worker_info_t *worker, int job_id,
                     const char *filename, const char *keyword);
static int send_terminate(worker_info_t *worker);
static int dispatch_initial_tasks(worker_info_t workers[],
                                  int num_workers,
                                  char *files[],
                                  int total_files,
                                  const char *keyword,
                                  int *next_file_index,
                                  int *next_job_id);
static int find_result_source(worker_info_t workers[],
                              int num_workers,
                              result_msg_t *result);
static void print_summary(int total_files, int files_with_matches, int total_matches);
static void cleanup_workers(worker_info_t workers[], int num_workers);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *keyword = argv[1];
    char **files = &argv[2];
    int total_files = argc - 2;

    worker_info_t workers[NUM_WORKERS];
    memset(workers, 0, sizeof(workers));

    for (int i = 0; i < NUM_WORKERS; i++) {
        workers[i].worker_id = i;
        workers[i].busy = 0;
        workers[i].alive = 1;

        if (spawn_worker(&workers[i]) < 0) {
            fprintf(stderr, "Failed to spawn worker %d\n", i);
            cleanup_workers(workers, i);
            return EXIT_FAILURE;
        }
    }

    int next_file_index = 0;
    int next_job_id = 1;

    if (dispatch_initial_tasks(workers, NUM_WORKERS, files, total_files,
                               keyword, &next_file_index, &next_job_id) < 0) {
        fprintf(stderr, "Failed to dispatch initial tasks\n");
        cleanup_workers(workers, NUM_WORKERS);
        return EXIT_FAILURE;
    }

    int completed_jobs = 0;
    int files_with_matches = 0;
    int total_matches = 0;

    while (completed_jobs < total_files) {
        result_msg_t result;
        int worker_idx = find_result_source(workers, NUM_WORKERS, &result);
        if (worker_idx < 0) {
            fprintf(stderr, "Failed to receive result from any worker\n");
            break;
        }

        workers[worker_idx].busy = 0;
        completed_jobs++;

        if (result.status == 0) {
            printf("%s: %d matches (worker %d)\n",
                   result.filename, result.match_count, result.worker_id);

            total_matches += result.match_count;
            if (result.match_count > 0) {
                files_with_matches++;
            }
        } else {
            printf("%s: failed to process\n", result.filename);
        }

        if (next_file_index < total_files) {
            if (send_task(&workers[worker_idx], next_job_id,
                          files[next_file_index], keyword) < 0) {
                fprintf(stderr, "Failed to send task to worker %d\n", worker_idx);
                workers[worker_idx].alive = 0;
            } else {
                workers[worker_idx].busy = 1;
                next_file_index++;
                next_job_id++;
            }
        }
    }

    for (int i = 0; i < NUM_WORKERS; i++) {
        if (workers[i].alive) {
            if (send_terminate(&workers[i]) < 0) {
                fprintf(stderr, "Warning: failed to terminate worker %d cleanly\n", i);
            }
        }
    }

    cleanup_workers(workers, NUM_WORKERS);
    print_summary(total_files, files_with_matches, total_matches);

    return EXIT_SUCCESS;
}

static void usage(const char *progname) {
    fprintf(stderr, "Usage: %s <keyword> <file1> [file2 ...]\n", progname);
}

static int spawn_worker(worker_info_t *worker) {
    int task_pipe[2];
    int result_pipe[2];

    if (pipe(task_pipe) < 0) {
        perror("pipe task_pipe");
        return -1;
    }

    if (pipe(result_pipe) < 0) {
        perror("pipe result_pipe");
        close(task_pipe[0]);
        close(task_pipe[1]);
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");

        close(task_pipe[0]);
        close(task_pipe[1]);
        close(result_pipe[0]);
        close(result_pipe[1]);
        return -1;
    }

    if (pid == 0) {
        // child
        close(task_pipe[1]);     // child reads tasks
        close(result_pipe[0]);   // child writes results

        run_worker(worker->worker_id, task_pipe[0], result_pipe[1]);

        close(task_pipe[0]);
        close(result_pipe[1]);
        _exit(EXIT_SUCCESS);
    }

    // parent
    close(task_pipe[0]);       // parent writes tasks
    close(result_pipe[1]);     // parent reads results

    worker->task_write_fd = task_pipe[1];
    worker->result_read_fd = result_pipe[0];
    worker->pid = pid;

    return 0;
}

static int send_task(worker_info_t *worker, int job_id,
                     const char *filename, const char *keyword) {
    task_msg_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.job_id = job_id;
    msg.terminate = 0;

    strncpy(msg.filename, filename, MAX_FILENAME - 1);
    strncpy(msg.keyword, keyword, MAX_KEYWORD - 1);

    ssize_t n = write_full(worker->task_write_fd, &msg, sizeof(msg));
    if (n != (ssize_t)sizeof(msg)) {
        perror("write_full send_task");
        return -1;
    }

    return 0;
}

static int send_terminate(worker_info_t *worker) {
    task_msg_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.job_id = -1;
    msg.terminate = 1;

    ssize_t n = write_full(worker->task_write_fd, &msg, sizeof(msg));
    if (n != (ssize_t)sizeof(msg)) {
        perror("write_full send_terminate");
        return -1;
    }

    return 0;
}

static int dispatch_initial_tasks(worker_info_t workers[],
                                  int num_workers,
                                  char *files[],
                                  int total_files,
                                  const char *keyword,
                                  int *next_file_index,
                                  int *next_job_id) {
    for (int i = 0; i < num_workers && *next_file_index < total_files; i++) {
        if (!workers[i].alive) {
            continue;
        }

        if (send_task(&workers[i], *next_job_id, files[*next_file_index], keyword) < 0) {
            return -1;
        }

        workers[i].busy = 1;
        (*next_file_index)++;
        (*next_job_id)++;
    }

    return 0;
}

static int find_result_source(worker_info_t workers[],
                              int num_workers,
                              result_msg_t *result) {
    for (;;) {
        for (int i = 0; i < num_workers; i++) {
            if (!workers[i].alive || !workers[i].busy) {
                continue;
            }

            ssize_t n = read(workers[i].result_read_fd, result, sizeof(*result));
            if (n == (ssize_t)sizeof(*result)) {
                return i;
            } else if (n == 0) {
                fprintf(stderr, "Worker %d closed result pipe unexpectedly\n", i);
                workers[i].alive = 0;
                workers[i].busy = 0;
            } else if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("read result");
                workers[i].alive = 0;
                workers[i].busy = 0;
            } else {
                fprintf(stderr, "Partial read from worker %d\n", i);
                workers[i].alive = 0;
                workers[i].busy = 0;
            }
        }

        int any_busy = 0;
        for (int i = 0; i < num_workers; i++) {
            if (workers[i].alive && workers[i].busy) {
                any_busy = 1;
                break;
            }
        }

        if (!any_busy) {
            return -1;
        }
    }
}

static void print_summary(int total_files, int files_with_matches, int total_matches) {
    printf("\nSummary:\n");
    printf("Total files processed: %d\n", total_files);
    printf("Files with matches: %d\n", files_with_matches);
    printf("Total matches: %d\n", total_matches);
}

static void cleanup_workers(worker_info_t workers[], int num_workers) {
    for (int i = 0; i < num_workers; i++) {
        if (workers[i].task_write_fd > 0) {
            close(workers[i].task_write_fd);
            workers[i].task_write_fd = -1;
        }
        if (workers[i].result_read_fd > 0) {
            close(workers[i].result_read_fd);
            workers[i].result_read_fd = -1;
        }
    }

    for (int i = 0; i < num_workers; i++) {
        if (workers[i].pid > 0) {
            waitpid(workers[i].pid, NULL, 0);
        }
    }
}