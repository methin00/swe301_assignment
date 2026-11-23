#define _POSIX_C_SOURCE 200809L

#include "signal_manager.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    sigset_t managed, wait_mask;

    if (sigemptyset(&managed) == -1 ||
        sigaddset(&managed, SIGINT) == -1 ||
        sigaddset(&managed, SIGTERM) == -1 ||
        sigaddset(&managed, SIGUSR1) == -1 ||
        sigprocmask(SIG_BLOCK, &managed, &wait_mask) == -1) {
        perror("block watchdog signals");
        return EXIT_FAILURE;
    }

    if (signal_manager_init() == -1) {
        perror("install signal handlers");
        return EXIT_FAILURE;
    }

    /* Unblock the managed signals only while waiting for an event. */
    if (sigdelset(&wait_mask, SIGINT) == -1 ||
        sigdelset(&wait_mask, SIGTERM) == -1 ||
        sigdelset(&wait_mask, SIGUSR1) == -1) {
        perror("prepare signal mask");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Watchdog started (PID %ld)\n", (long)getpid());
    while (!shutdown_requested()) {
        int report = take_report_request();
        if (report == -1) {
            perror("read report request");
            return EXIT_FAILURE;
        }
        if (report)
            fprintf(stderr, "Status report requested\n");

        if (sigsuspend(&wait_mask) == -1 && errno != EINTR) {
            perror("wait for signal");
            return EXIT_FAILURE;
        }
    }

    fprintf(stderr, "Watchdog shutting down\n");
    return EXIT_SUCCESS;
}
