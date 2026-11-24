#define _POSIX_C_SOURCE 200809L

#include "logger.h"

#include <stdio.h>
#include <time.h>

static int write_timestamp(FILE *file)
{
    time_t now = time(NULL);
    struct tm current_time;
    char timestamp[32];

    if (now == (time_t)-1) {
        return -1;
    }

    if (localtime_r(&now, &current_time) == NULL) {
        return -1;
    }

    if (strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", &current_time) == 0) {
        return -1;
    }

    return fprintf(file, "%s", timestamp) < 0 ? -1 : 0;
}

int logger_log_event(
    const char *log_path,
    const char *event,
    pid_t pid,
    const char *details
)
{
    if (log_path == NULL || event == NULL) {
        return -1;
    }

    FILE *file = fopen(log_path, "a");
    if (file == NULL) {
        return -1;
    }

    const char *safe_details = details != NULL ? details : "-";
    int failed = 0;

    if (write_timestamp(file) < 0 ||
        fprintf(file, " | %s | pid=%ld | %s\n",
                event, (long)pid, safe_details) < 0) {
        failed = 1;
    }

    if (fclose(file) != 0) {
        failed = 1;
    }

    return failed ? -1 : 0;
}

