#define _POSIX_C_SOURCE 200809L

#include "logger.h"
#include "process_manager.h"
#include "signal_manager.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static const char log_path[] = "watchdog.log";

static int log_running_workers(const char *event)
{
    const Worker *workers = process_manager_workers();
    for (int i = 0; i < WORKER_COUNT; ++i) {
        if (workers[i].running &&
            logger_log_event(log_path, event, workers[i].pid, "running") == -1)
            return -1;
    }
    return 0;
}

static int run_loop(const sigset_t *wait_mask)
{
    while (!shutdown_requested()) {
        int child_event = process_manager_take_child_event();
        if (child_event == -1 || (child_event && process_manager_reap() == -1))
            return -1;

        int periodic_event = process_manager_take_periodic_event();
        if (periodic_event == -1 || (periodic_event && process_manager_periodic_check() == -1))
            return -1;

        int report = take_report_request();
        if (report == -1 || (report && log_running_workers("status") == -1))
            return -1;

        if (sigsuspend(wait_mask) == -1 && errno != EINTR)
            return -1;
    }
    return 0;
}

static int stop_workers(void)
{
    const Worker *workers = process_manager_workers();
    const struct timespec interval = { .tv_nsec = 100000000L };
    const int grace_intervals = 20;
    int tick = 0;

    for (;;) {
        if (process_manager_reap() == -1)
            return -1;

        int running = 0;
        for (int i = 0; i < WORKER_COUNT; ++i) {
            if (!workers[i].running)
                continue;
            ++running;
            if (tick == 0 || tick == grace_intervals) {
                int signo = tick == 0 ? SIGTERM : SIGKILL;
                if (kill(workers[i].pid, signo) == -1 && errno != ESRCH)
                    return -1;
            }
        }
        if (!running)
            return 0;

        struct timespec remaining = interval;
        while (nanosleep(&remaining, &remaining) == -1) {
            if (errno != EINTR)
                return -1;
        }
        if (tick <= grace_intervals)
            ++tick;
    }
}

int main(void)
{
    sigset_t managed, wait_mask;

    if (sigemptyset(&managed) == -1 ||
        sigaddset(&managed, SIGALRM) == -1 ||
        sigaddset(&managed, SIGCHLD) == -1 ||
        sigaddset(&managed, SIGINT) == -1 ||
        sigaddset(&managed, SIGTERM) == -1 ||
        sigaddset(&managed, SIGUSR1) == -1 ||
        sigprocmask(SIG_BLOCK, &managed, &wait_mask) == -1) {
        perror("block watchdog signals");
        return EXIT_FAILURE;
    }

    if (signal_manager_init() == -1 || process_manager_init_signals() == -1) {
        perror("install signal handlers");
        return EXIT_FAILURE;
    }

    /* Unblock the managed signals only while waiting for an event. */
    if (sigdelset(&wait_mask, SIGALRM) == -1 ||
        sigdelset(&wait_mask, SIGCHLD) == -1 ||
        sigdelset(&wait_mask, SIGINT) == -1 ||
        sigdelset(&wait_mask, SIGTERM) == -1 ||
        sigdelset(&wait_mask, SIGUSR1) == -1) {
        perror("prepare signal mask");
        return EXIT_FAILURE;
    }

    if (process_manager_start() == -1) {
        perror("start workers");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Watchdog started (PID %ld)\n", (long)getpid());
    int result = EXIT_SUCCESS;
    if (process_manager_start_timer() == -1) {
        perror("start timer");
        result = EXIT_FAILURE;
    } else if (log_running_workers("worker_started") == -1 || run_loop(&wait_mask) == -1) {
        perror("watchdog");
        result = EXIT_FAILURE;
    }

    process_manager_disable_restart();
    if (process_manager_stop_timer() == -1) {
        perror("stop timer");
        result = EXIT_FAILURE;
    }
    if (stop_workers() == -1) {
        perror("stop workers");
        return EXIT_FAILURE;
    }
    if (logger_log_event(log_path, "shutdown", getpid(), "workers reaped") == -1) {
        perror("log shutdown");
        return EXIT_FAILURE;
    }
    return result;
}
