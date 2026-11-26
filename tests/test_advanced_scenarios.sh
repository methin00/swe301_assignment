#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

make

TMP_DIR="$(mktemp -d)"
LOG_FILE="$ROOT_DIR/watchdog.log"
SAVED_WORKER="$TMP_DIR/worker"
HAD_LOG=0
ACTIVE_PID=""

cp "$ROOT_DIR/worker" "$SAVED_WORKER"
if [[ -f "$LOG_FILE" ]]; then
    cp "$LOG_FILE" "$TMP_DIR/watchdog.log"
    HAD_LOG=1
fi

cleanup() {
    if [[ -n "$ACTIVE_PID" ]] && kill -0 "$ACTIVE_PID" 2>/dev/null; then
        kill -TERM "$ACTIVE_PID" 2>/dev/null || true
        wait "$ACTIVE_PID" 2>/dev/null || true
    fi

    cp "$SAVED_WORKER" "$ROOT_DIR/worker"
    mkdir -p "$ROOT_DIR/docs"
    cp "$LOG_FILE" "$ROOT_DIR/docs/sample_watchdog.log"

    if [[ "$HAD_LOG" -eq 1 ]]; then
        cp "$TMP_DIR/watchdog.log" "$LOG_FILE"
    fi

    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

gcc -std=c11 -Wall -Wextra -Wpedantic \
    "$ROOT_DIR/tests/test_worker.c" -o "$ROOT_DIR/worker"

: > "$LOG_FILE"

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

    for ((attempt = 0; attempt < 150; attempt++)); do
        if (( $(event_count_since "$start_line" "$event") >= expected )); then
            return 0
        fi
        sleep 0.1
    done

    echo "FAIL: $event olayları zamanında oluşmadı." >&2
    return 1
}

stop_watchdog() {
    kill -TERM "$ACTIVE_PID"
    wait "$ACTIVE_PID"
    ACTIVE_PID=""
}

# Normal çıkış: kod 0 ile biten worker restart edilmemeli.
start_line=$(wc -l < "$LOG_FILE")
WORKER_TEST_MODE=normal ./watchdog >"$TMP_DIR/normal.out" 2>&1 &
ACTIVE_PID=$!
wait_for_events "$start_line" "worker_exit" 2

if (( $(event_count_since "$start_line" "restart_attempt") != 0 )); then
    echo "FAIL: normal çıkan worker yeniden başlatıldı." >&2
    exit 1
fi

stop_watchdog
echo "PASS: kod 0 ile çıkan iki worker restart edilmedi."

# Hatalı çıkış: her worker üç kez restart edilmeli, dördüncü kayıpta durmalı.
start_line=$(wc -l < "$LOG_FILE")
WORKER_TEST_MODE=failure ./watchdog >"$TMP_DIR/failure.out" 2>&1 &
ACTIVE_PID=$!
wait_for_events "$start_line" "restart_limit" 2
wait_for_events "$start_line" "periodic_check" 2

started=$(event_count_since "$start_line" "worker_started")
restarted=$(event_count_since "$start_line" "worker_restarted")
attempts=$(event_count_since "$start_line" "restart_attempt")
limits=$(event_count_since "$start_line" "restart_limit")

if [[ "$started" -ne 2 || "$restarted" -ne 6 || "$attempts" -ne 6 || "$limits" -ne 2 ]]; then
    echo "FAIL: beklenen başlangıç/restart/sınır sayıları oluşmadı: $started/$attempts/$limits" >&2
    exit 1
fi

stop_watchdog
echo "PASS: iki worker üçer kez restart edildi; dördüncü kayıpta yenisi açılmadı."
echo "PASS: 5 saniyelik periyodik kontroller loglandı."

# Beklenmedik sinyal ile çıkış: worker yeniden başlatılmalı.
start_line=$(wc -l < "$LOG_FILE")
WORKER_TEST_MODE=wait ./watchdog >"$TMP_DIR/signal.out" 2>&1 &
ACTIVE_PID=$!
wait_for_events "$start_line" "worker_started" 2

mapfile -t worker_pids < <(
    awk -F'pid=' -v start="$start_line" \
        'NR > start && /worker_started/ {
            split($2, part, /[^0-9]/)
            print part[1]
        }' "$LOG_FILE"
)

if [[ "${#worker_pids[@]}" -ne 2 ]]; then
    echo "FAIL: iki worker PID'si bulunamadı." >&2
    exit 1
fi

kill -KILL "${worker_pids[0]}"
wait_for_events "$start_line" "worker_signal" 1
wait_for_events "$start_line" "restart_attempt" 1
wait_for_events "$start_line" "worker_restarted" 1
stop_watchdog

echo "PASS: SIGKILL ile beklenmedik çıkan worker yeniden başlatıldı."
echo "Tüm gelişmiş test senaryoları başarılı."

