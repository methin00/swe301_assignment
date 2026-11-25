#define _POSIX_C_SOURCE 200809L

#include "process_manager.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

static Worker workers[WORKER_COUNT];

static int start_worker(int index)
{
    pid_t pid = fork();

    if (pid == -1)
        return -1;

    if (pid == 0) {
        sigset_t mask;

        if (sigemptyset(&mask) == -1 ||
            sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
            _exit(126);

        execl("./worker", "worker", (char *)NULL);

        _exit(127);
    }

    workers[index].pid = pid;
    workers[index].running = 1;

    return 0;
}

int process_manager_start(void)
{
    for (int i = 0; i < WORKER_COUNT; ++i) {
        if (start_worker(i) == -1) {
            int saved = errno;

            for (int j = 0; j < i; ++j) {
                kill(workers[j].pid, SIGTERM);

                while (waitpid(workers[j].pid, NULL, 0) == -1 &&
                       errno == EINTR) {
                }

                workers[j].running = 0;
            }

            errno = saved;
            return -1;
        }
    }

    return 0;
}

const Worker *process_manager_workers(void)
{
    return workers;
}
