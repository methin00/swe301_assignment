# Watchdog and Worker

## Build

From the repository root on Linux, run:

```sh
make
```

This builds two executables, `watchdog` and `worker`.

## Run

```sh
./watchdog
```

The watchdog starts two worker processes, prints its own PID, and writes events to `watchdog.log` in the current directory.

## Signals

In a second terminal, replace `<WATCHDOG_PID>` with the PID printed at startup. To log the PID and state of each running worker, send:

```sh
kill -USR1 <WATCHDOG_PID>
```

To shut down the system, send `SIGTERM` to the watchdog:

```sh
kill -TERM <WATCHDOG_PID>
```

You can also press Ctrl+C in the terminal where the watchdog is running in the foreground. The workers ignore the terminal's `SIGINT`. The watchdog sends them `SIGTERM` and waits for them to exit.

## Run the tests

```sh
./tests/test_scenarios.sh
```

This script checks that two workers start, that the `SIGUSR1` report contains their PIDs, and that the watchdog reaps both workers on `SIGTERM` or `SIGINT`. See [the sample log](docs/sample_watchdog.log) for example output.

```sh
./tests/test_advanced_scenarios.sh
```

This script checks normal exit (code 0), error exit (code 7), the limit of three restarts per worker, recovery after `SIGKILL`, and the five-second periodic check. In a separate foreground test, pressing Ctrl+C shut down the watchdog and both workers exited on `SIGTERM` (signal 15). See [the test results](docs/test_results.md).
