#!/usr/bin/env bats
# pi-carplay-supervise: restarts, back-off, crash records, single instance, stop.

setup() {
  SUPERVISE="$BATS_TEST_DIRNAME/../../deploy/bin/pi-carplay-supervise"
  export PI_CARPLAY_STATE_DIR="$BATS_TEST_TMPDIR/state"
  export PI_CARPLAY_RUN_DIR="$BATS_TEST_TMPDIR/run"
  export PI_CARPLAY_MAX_DELAY=1
  unset WAYLAND_DISPLAY
  WORK=$BATS_TEST_TMPDIR
}

teardown() {
  "$SUPERVISE" --stop t 2>/dev/null || true
}

wait_for() {
  for _ in $(seq 1 100); do
    eval "$1" && return 0
    sleep 0.1
  done
  return 1
}

@test "restarts a program that keeps crashing and records each exit" {
  "$SUPERVISE" t sh -c "echo run >> '$WORK/runs'; exit 3" &
  wait_for "[ \$(wc -l < '$WORK/runs' 2>/dev/null || echo 0) -ge 3 ]"
  "$SUPERVISE" --stop t
  grep -q "t exited with 3 after" "$PI_CARPLAY_STATE_DIR/crashes.log"
  grep -q "restarting in" "$PI_CARPLAY_STATE_DIR/t.log"
}

@test "names the signal that killed the program" {
  "$SUPERVISE" t sh -c 'kill -SEGV $$' &
  wait_for "grep -q 'killed by SEGV' '$PI_CARPLAY_STATE_DIR/crashes.log' 2>/dev/null"
}

@test "captures program output in its log" {
  "$SUPERVISE" t sh -c 'echo hello-from-child; sleep 30' &
  wait_for "grep -q hello-from-child '$PI_CARPLAY_STATE_DIR/t.log' 2>/dev/null"
}

@test "--stop ends the supervisor and the program" {
  "$SUPERVISE" t sh -c "echo \$\$ > '$WORK/child'; exec sleep 60" &
  wait_for "[ -s '$WORK/child' ]"
  child=$(cat "$WORK/child")
  run "$SUPERVISE" --stop t
  [ "$status" -eq 0 ]
  wait_for "! kill -0 $child 2>/dev/null"
  [ ! -e "$PI_CARPLAY_RUN_DIR/t.pid" ]
}

@test "a second supervisor for the same name does not start" {
  "$SUPERVISE" t sleep 60 &
  wait_for "[ -s '$PI_CARPLAY_RUN_DIR/t.pid' ]"
  run "$SUPERVISE" t sh -c "touch '$WORK/second'"
  [ "$status" -eq 0 ]
  [[ "$output" == *"already supervised"* ]]
  [ ! -e "$WORK/second" ]
}

@test "a stale pid file from a reused pid is ignored" {
  mkdir -p "$PI_CARPLAY_RUN_DIR"
  sleep 60 &
  echo $! > "$PI_CARPLAY_RUN_DIR/t.pid"
  "$SUPERVISE" t sh -c "touch '$WORK/started'; sleep 60" &
  wait_for "[ -e '$WORK/started' ]"
  kill %1
}

@test "stops restarting once the Wayland session is gone" {
  export WAYLAND_DISPLAY="$WORK/no-such-socket"
  run "$SUPERVISE" t sh -c "touch '$WORK/ran'"
  [ "$status" -eq 0 ]
  [ ! -e "$WORK/ran" ]
  grep -q "session ended" "$PI_CARPLAY_STATE_DIR/t.log"
}

@test "rotates a log that outgrew its limit" {
  export PI_CARPLAY_MAX_LOG_BYTES=100
  mkdir -p "$PI_CARPLAY_STATE_DIR"
  head -c 1000 /dev/zero > "$PI_CARPLAY_STATE_DIR/t.log"
  "$SUPERVISE" t sleep 60 &
  wait_for "[ -e '$PI_CARPLAY_STATE_DIR/t.log.1' ]"
}

@test "caps the crash record" {
  export PI_CARPLAY_MAX_CRASH_LINES=3
  "$SUPERVISE" t false &
  wait_for "[ \$(wc -l < '$PI_CARPLAY_STATE_DIR/crashes.log' 2>/dev/null || echo 0) -ge 3 ]"
  sleep 2.5
  "$SUPERVISE" --stop t
  [ "$(wc -l < "$PI_CARPLAY_STATE_DIR/crashes.log")" -le 3 ]
}

@test "rejects names that could escape the state directory" {
  run "$SUPERVISE" ../evil true
  [ "$status" -eq 2 ]
  run "$SUPERVISE" --stop "a b"
  [ "$status" -eq 2 ]
}

@test "--stop of something not running succeeds" {
  run "$SUPERVISE" --stop nothing
  [ "$status" -eq 0 ]
}

@test "--status lists supervised programs" {
  "$SUPERVISE" t sleep 60 &
  wait_for "[ -s '$PI_CARPLAY_RUN_DIR/t.pid' ]"
  run "$SUPERVISE" --status
  [[ "$output" == *"t running"* ]]
}

@test "usage error without a command" {
  run "$SUPERVISE" onlyname
  [ "$status" -eq 2 ]
}
