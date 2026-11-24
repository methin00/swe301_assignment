#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

#define WORKER_COUNT 2

typedef struct {
    pid_t pid;
    int running;
    unsigned int restart_count;
} Worker;

int process_manager_start(void);
const Worker *process_manager_workers(void);

int process_manager_reap(void);
int process_manager_init_signals(void);
int process_manager_take_child_event(void);

#endif
