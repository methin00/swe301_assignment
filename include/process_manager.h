#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

#define WORKER_COUNT 2
#define MAX_RESTARTS 3
#define CHECK_INTERVAL 5

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

void process_manager_disable_restart(void);

int process_manager_start_timer(void);
int process_manager_stop_timer(void);
int process_manager_take_periodic_event(void);
int process_manager_periodic_check(void);

#endif
