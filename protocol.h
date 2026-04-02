#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_FILENAME 256
#define MAX_KEYWORD 64

typedef struct {
    int job_id;
    int terminate;
    char filename[MAX_FILENAME];
    char keyword[MAX_KEYWORD];
} task_msg_t;

typedef struct {
    int job_id;
    int worker_id;
    int match_count;
    int status;   // 0 success, -1 failure
    char filename[MAX_FILENAME];
} result_msg_t;

#endif