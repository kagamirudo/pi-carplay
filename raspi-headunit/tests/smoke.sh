#!/bin/bash
# Start the real headunit binary (offscreen, --smoke) under every config state
# a car can leave behind, and check it always comes up.
#
#   tests/smoke.sh <path-to-headunit>
set -uo pipefail

BIN=${1:?usage: smoke.sh <headunit>}
WORK=$(mktemp -d)
trap 'chmod -R u+rwx "$WORK"; rm -rf "$WORK"' EXIT
export QT_QPA_PLATFORM=offscreen HEADUNIT_NO_BLUETOOTH=1
failures=0

# run <name> <expected-exit> [env assignments...] -- [args...]
run() {
  local name=$1 want=$2
  shift 2
  local envs=()
  while [ $# -gt 0 ] && [ "$1" != -- ]; do envs+=("$1"); shift; done
  [ "${1:-}" = -- ] && shift
  local out code
  out=$(cd "$WORK/cwd" && env "${envs[@]}" timeout 30 "$BIN" --smoke "$@" 2>&1)
  code=$?
  if [ "$code" -eq "$want" ]; then
    echo "ok   $name"
  else
    echo "FAIL $name: exit $code, expected $want"
    printf '%s\n' "$out" | tail -n 20 | sed 's/^/     /'
    failures=$((failures + 1))
  fi
}

fresh() {
  rm -rf "${WORK:?}/home" "${WORK:?}/cwd"
  mkdir -p "$WORK/home/.config/raspi-headunit" "$WORK/cwd"
  CFG=$WORK/home/.config/raspi-headunit/headunit.json
}

H=(HOME="$WORK/home" XDG_CONFIG_HOME=)

fresh
run "first boot, no config" 0 "${H[@]}" --

fresh
printf '{"display":{"output":"SPI-1"},"dashcam":{"segmentSeconds":60}}' > "$CFG"
run "valid config" 0 "${H[@]}" --

for case in "corrupt:{\"display\":" "empty:" "binary:$(printf '\x00\xff\x13')" "wrong type:{\"display\":{\"width\":\"big\"}}"; do
  fresh
  printf '%s' "${case#*:}" > "$CFG"
  run "${case%%:*} config is set aside" 0 "${H[@]}" --
  if [ -e "$CFG" ] || [ ! -e "$CFG.corrupt" ]; then
    echo "FAIL ${case%%:*} config was not moved to headunit.json.corrupt"
    failures=$((failures + 1))
  fi
done

fresh
head -c 200000 /dev/zero | tr '\0' '{' > "$CFG"
run "huge config is set aside" 0 "${H[@]}" --

if [ "$(id -u)" -ne 0 ]; then
  fresh
  echo '{}' > "$CFG"
  chmod 000 "$CFG"
  run "unreadable config" 0 "${H[@]}" --
fi

fresh
run "XDG_CONFIG_HOME wins" 0 HOME=/nonexistent XDG_CONFIG_HOME="$WORK/home/.config" --

fresh
run "no HOME" 0 -u HOME -u XDG_CONFIG_HOME --

fresh
printf 'nope' > "$WORK/bad.json"
run "explicit bad --config fails fast" 2 "${H[@]}" -- --config "$WORK/bad.json"

fresh
run "explicit missing --config starts with defaults" 0 "${H[@]}" -- --config "$WORK/none.json"

fresh
run "--help" 0 "${H[@]}" -- --help

if [ "$failures" -ne 0 ]; then
  echo "$failures smoke case(s) failed"
  exit 1
fi
echo "all smoke cases passed"
