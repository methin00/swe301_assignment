#define _POSIX_C_SOURCE 200809L

#include <unistd.h>

int main(void)
{
    for (;;) {
        pause();
    }
}
