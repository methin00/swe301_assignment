#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

make

LOG_FILE="$ROOT_DIR/watchdog.log"
TMP_DIR="$(mktemp -d)"
SAVED_LOG="$TMP_DIR/watchdog.log"
HAD_LOG=0
ACTIVE_PID=""

if [[ -f "$LOG_FILE" ]]; then
    cp "$LOG_FILE" "$SAVED_LOG"
    HAD_LOG=1
fi

: > "$LOG_FILE"

cleanup() {
    if [[ -n "$ACTIVE_PID" ]] && kill -0 "$ACTIVE_PID" 2>/dev/null; then
        kill -TERM "$ACTIVE_PID" 2>/dev/null || true
        wait "$ACTIVE_PID" 2>/dev/null || true
    fi

    mkdir -p "$ROOT_DIR/docs"
    cp "$LOG_FILE" "$ROOT_DIR/docs/sample_watchdog.log"

    if [[ "$HAD_LOG" -eq 1 ]]; then
        cp "$SAVED_LOG" "$LOG_FILE"
    fi

    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

event_count_since() {
    local start_line="$1"
    local event="$2"

    awk -F'|' -v start="$start_line" -v event="$event" \
        'NR > start && index($2, event) { count++ }
         END { print count + 0 }' "$LOG_FILE"
}

wait_for_events() {
    local start_line="$1"
    local event="$2"
    local expected="$3"

    for ((attempt = 0; attempt < 50; attempt++)); do
        if (( $(event_count_since "$start_line" "$event") >= expected )); then
            return 0
        fi
        sleep 0.1
    done

    echo "FAIL: $event olayları zamanında oluşmadı." >&2
    return 1
}

worker_pids_since() {
    local start_line="$1"

    awk -F'pid=' -v start="$start_line" \
        'NR > start && /worker_started/ {
            split($2, part, /[^0-9]/)
            print part[1]
        }' "$LOG_FILE" | sort -nu
}

wait_for_workers_to_exit() {
    local -a pids=("$@")

    for pid in "${pids[@]}"; do
        for ((attempt = 0; attempt < 50; attempt++)); do
            if ! kill -0 "$pid" 2>/dev/null; then
                break
            fi
            sleep 0.1
        done

        if kill -0 "$pid" 2>/dev/null; then
            echo "FAIL: worker PID $pid hâlâ çalışıyor." >&2
            return 1
        fi
    done
}

run_shutdown_case() {
    local signal_name="$1"
    local signal_flag="$2"
    local start_line
    local watchdog_pid
    local worker_pid_text
    local status_pid_text
    local expected_pid_text
    local -a worker_pids

    start_line=$(wc -l < "$LOG_FILE")

    ./watchdog >"$TMP_DIR/watchdog-$signal_name.out" 2>&1 &
    ACTIVE_PID=$!
    watchdog_pid="$ACTIVE_PID"

    wait_for_events "$start_line" "worker_started" 2
    mapfile -t worker_pids < <(worker_pids_since "$start_line")

    if [[ "${#worker_pids[@]}" -ne 2 ]]; then
        echo "FAIL: iki farklı worker PID'si bulunamadı." >&2
        return 1
    fi

    for pid in "${worker_pids[@]}"; do
        if ! kill -0 "$pid" 2>/dev/null; then
            echo "FAIL: worker PID $pid çalışmıyor." >&2
            return 1
        fi
    done

    echo "PASS: iki worker başladı; PID'ler: ${worker_pids[*]}"

    if [[ "$signal_flag" == "report" ]]; then
        kill -USR1 "$watchdog_pid"
        wait_for_events "$start_line" "status" 2

        status_pid_text=$(
            awk -F'pid=' -v start="$start_line" \
                'NR > start && /status/ {
                    split($2, part, /[^0-9]/)
                    print part[1]
                }' "$LOG_FILE" | sort -nu
        )
        expected_pid_text=$(printf '%s\n' "${worker_pids[@]}" | sort -nu)

        if [[ "$status_pid_text" != "$expected_pid_text" ]]; then
            echo "FAIL: durum raporundaki PID'ler worker PID'leriyle eşleşmedi." >&2
            return 1
        fi

        echo "PASS: SIGUSR1 raporunda iki worker PID'si loglandı."
    fi

    # Sinyal yalnızca watchdog PID'sine gönderilir.
    kill "-$signal_name" "$watchdog_pid"
    wait "$watchdog_pid"
    ACTIVE_PID=""

    wait_for_workers_to_exit "${worker_pids[@]}"
    wait_for_events "$start_line" "shutdown" 1

    echo "PASS: SIG$signal_name ile watchdog kapandı ve worker süreçleri toplandı."
}

run_shutdown_case TERM report
run_shutdown_case INT no-report

echo "Tüm mevcut test senaryoları başarılı."
