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

int process_manager_reap(void)
{
    int status;
    pid_t pid;

    for (;;) {
        pid = waitpid(-1, &status, WNOHANG);

        if (pid == 0)
            return 0;

        if (pid == -1) {
            if (errno == EINTR)
                continue;

            if (errno == ECHILD)
                return 0;

            return -1;
        }

        for (int i = 0; i < WORKER_COUNT; ++i) {
            if (workers[i].pid != pid)
                continue;

            workers[i].running = 0;

            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);

                if (exit_code == 0) {
                    fprintf(stderr,
                            "Worker %d exited normally (PID %ld)\n",
                            i + 1, (long)pid);
                } else {
                    fprintf(stderr,
                            "Worker %d exited with code %d (PID %ld)\n",
                            i + 1, exit_code, (long)pid);
                }
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr,
                        "Worker %d terminated by signal %d (PID %ld)\n",
                        i + 1, WTERMSIG(status), (long)pid);
            }

            break;
        }
    }
}

static volatile sig_atomic_t child_event_pending;

static void handle_child_signal(int signo)
{
    (void)signo;
    child_event_pending = 1;
}

int process_manager_init_signals(void)
{
    struct sigaction action = {0};

    action.sa_handler = handle_child_signal;
    action.sa_flags = SA_NOCLDSTOP;

    if (sigemptyset(&action.sa_mask) == -1)
        return -1;

    return sigaction(SIGCHLD, &action, NULL);
}

int process_manager_take_child_event(void)
{
    sigset_t blocked, previous;
    int pending;

    if (sigemptyset(&blocked) == -1 ||
        sigaddset(&blocked, SIGCHLD) == -1 ||
        sigprocmask(SIG_BLOCK, &blocked, &previous) == -1)
        return -1;

    pending = child_event_pending;
    child_event_pending = 0;

    if (sigprocmask(SIG_SETMASK, &previous, NULL) == -1)
        return -1;

    return pending;
}
