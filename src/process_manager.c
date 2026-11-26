#define _POSIX_C_SOURCE 200809L

#include "process_manager.h"
#include "logger.h"
#include "signal_manager.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

static Worker workers[WORKER_COUNT];
static int restart_enabled = 1;
static const char log_path[] = "watchdog.log";

static volatile sig_atomic_t child_event_pending;

static void log_event(const char *event, pid_t pid,
                      const char *details)
{
    if (logger_log_event(log_path, event, pid, details) == -1)
        fprintf(stderr, "Failed to write watchdog log\n");
}

static int start_worker(int index)
{
    pid_t pid = fork();

    if (pid == -1)
        return -1;

    if (pid == 0) {
        sigset_t mask;
        struct sigaction action = {0};

        action.sa_handler = SIG_IGN;

        if (sigemptyset(&action.sa_mask) == -1 ||
            sigaction(SIGINT, &action, NULL) == -1)
            _exit(126);

        action.sa_handler = SIG_DFL;

        if (sigaction(SIGTERM, &action, NULL) == -1)
            _exit(126);

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

void process_manager_disable_restart(void)
{
    restart_enabled = 0;
}

static int can_restart(void)
{
    sigset_t pending;

    if (!restart_enabled || shutdown_requested())
        return 0;

    if (sigpending(&pending) == -1)
        return 0;

    if (sigismember(&pending, SIGINT) == 1 ||
        sigismember(&pending, SIGTERM) == 1)
        return 0;

    return 1;
}

static void restart_worker(int index)
{
    char details[128];
    pid_t old_pid = workers[index].pid;

    while (can_restart() &&
           workers[index].restart_count < MAX_RESTARTS) {

        workers[index].restart_count++;

        snprintf(details, sizeof(details),
                 "Worker %d restart attempt %u/%d",
                 index + 1,
                 workers[index].restart_count,
                 MAX_RESTARTS);

        log_event("restart_attempt", old_pid, details);

        if (start_worker(index) == 0) {
            log_event("worker_restarted",
                      workers[index].pid, details);

            fprintf(stderr,
                    "%s successful (PID %ld)\n",
                    details, (long)workers[index].pid);

            return;
        }

        log_event("restart_failed", old_pid, details);
    }

    if (can_restart() &&
        workers[index].restart_count >= MAX_RESTARTS) {

        snprintf(details, sizeof(details),
                 "Worker %d reached restart limit (%d)",
                 index + 1, MAX_RESTARTS);

        log_event("restart_limit", old_pid, details);
        fprintf(stderr, "%s\n", details);
    }
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

            int unexpected = 0;
            char details[128];

            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);

                snprintf(details, sizeof(details),
                         "Worker %d exited with code %d",
                         i + 1, exit_code);

                unexpected = (exit_code != 0);

                log_event(unexpected ? "worker_error" :
                          "worker_exit", pid, details);

            } else if (WIFSIGNALED(status)) {
                snprintf(details, sizeof(details),
                         "Worker %d terminated by signal %d",
                         i + 1, WTERMSIG(status));

                unexpected = 1;

                log_event("worker_signal", pid, details);

            } else {
                break;
            }

            fprintf(stderr, "%s (PID %ld)\n",
                    details, (long)pid);

            if (unexpected)
                restart_worker(i);

            break;
        }
    }
}

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
