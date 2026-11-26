#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    const char *mode = getenv("WORKER_TEST_MODE");

    if (mode == NULL || strcmp(mode, "wait") == 0) {
        for (;;) {
            pause();
        }
    }

    if (strcmp(mode, "normal") == 0) {
        return 0;
    }

    if (strcmp(mode, "failure") == 0) {
        return 7;
    }

    return 2;
}

