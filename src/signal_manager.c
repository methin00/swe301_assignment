#define _POSIX_C_SOURCE 200809L

#include "signal_manager.h"

#include <signal.h>
#include <stddef.h>

static volatile sig_atomic_t shutdown_pending;
static volatile sig_atomic_t report_pending;

static void handle_signal(int signo)
{
    if (signo == SIGUSR1)
        report_pending = 1;
    else
        shutdown_pending = 1;
}

int signal_manager_init(void)
{
    struct sigaction action = {0};

    action.sa_handler = handle_signal;
    if (sigemptyset(&action.sa_mask) == -1 ||
        sigaction(SIGINT, &action, NULL) == -1 ||
        sigaction(SIGTERM, &action, NULL) == -1)
        return -1;

    return sigaction(SIGUSR1, &action, NULL);
}

int shutdown_requested(void)
{
    return shutdown_pending;
}

int take_report_request(void)
{
    sigset_t blocked, previous;
    int pending;

    /* Keep a new SIGUSR1 from being lost between the read and clear. */
    if (sigemptyset(&blocked) == -1 ||
        sigaddset(&blocked, SIGUSR1) == -1 ||
        sigprocmask(SIG_BLOCK, &blocked, &previous) == -1)
        return -1;

    pending = report_pending;
    report_pending = 0;
    if (sigprocmask(SIG_SETMASK, &previous, NULL) == -1)
        return -1;

    return pending;
}
