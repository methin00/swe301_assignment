CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude

WATCHDOG_SOURCES = src/main.c src/signal_manager.c src/process_manager.c src/logger.c
WORKER_SOURCE = workers/worker.c

.PHONY: all clean

all: watchdog worker

watchdog: $(WATCHDOG_SOURCES) include/signal_manager.h include/process_manager.h include/logger.h
	$(CC) $(CFLAGS) $(WATCHDOG_SOURCES) -o $@

worker: $(WORKER_SOURCE)
	$(CC) $(CFLAGS) $(WORKER_SOURCE) -o $@

clean:
	rm -f watchdog worker
