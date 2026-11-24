#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>

int logger_log_event(
    const char *log_path,
    const char *event,
    pid_t pid,
    const char *details
);

#endif

