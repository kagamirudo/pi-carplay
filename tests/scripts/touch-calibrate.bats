#!/usr/bin/env bats
# pi-carplay-touch-calibrate without a real panel: the --matrix/--reset/--show
# and --dry-run paths, driven through env overrides so nothing touches the system.

setup() {
  TOOL="$BATS_TEST_DIRNAME/../../tools/pi-carplay-touch-calibrate"
  export PI_CARPLAY_TOUCH_RULE="$BATS_TEST_TMPDIR/99-touch.rules"
  export LABWC_RC="$BATS_TEST_TMPDIR/rc.xml"
  export PI_CARPLAY_NO_RELOAD=1
  export SUDO=            # set-but-empty: the tool runs file ops without sudo
}

@test "a dry run with --matrix changes nothing" {
  run "$TOOL" --matrix '1 0 0 0 -1 1' --dry-run
  [ "$status" -eq 0 ]
  [[ "$output" == *"Would write"* ]]
  [ ! -e "$PI_CARPLAY_TOUCH_RULE" ]
}

@test "--matrix writes a udev rule with the calibration" {
  run "$TOOL" --matrix '1.05 0 -0.02 0 -0.98 1'
  [ "$status" -eq 0 ]
  [ -e "$PI_CARPLAY_TOUCH_RULE" ]
  grep -q 'ENV{LIBINPUT_CALIBRATION_MATRIX}="1.05 0 -0.02 0 -0.98 1"' "$PI_CARPLAY_TOUCH_RULE"
  grep -q 'ATTRS{name}=="ADS7846 Touchscreen"' "$PI_CARPLAY_TOUCH_RULE"
}

@test "--reset writes the shipped default matrix" {
  run "$TOOL" --reset
  [ "$status" -eq 0 ]
  grep -q 'LIBINPUT_CALIBRATION_MATRIX}="1 0 0 0 -1 1"' "$PI_CARPLAY_TOUCH_RULE"
}

@test "a bad matrix is refused and nothing is written" {
  run "$TOOL" --matrix '1 0 0 0 -1'
  [ "$status" -eq 1 ]
  [ ! -e "$PI_CARPLAY_TOUCH_RULE" ]
  run "$TOOL" --matrix '1 0 0 0 -1 x'
  [ "$status" -eq 1 ]
}

@test "--show reports the configured matrix" {
  "$TOOL" --matrix '1 0 0 0 -1 1'
  run "$TOOL" --show
  [ "$status" -eq 0 ]
  [[ "$output" == *"1 0 0 0 -1 1"* ]]
}

@test "a labwc calibrationMatrix is moved aside so udev is the one source" {
  mkdir -p "$(dirname "$LABWC_RC")"
  printf '<labwc_config><core><calibrationMatrix>9 9 9 9 9 9</calibrationMatrix></core></labwc_config>\n' > "$LABWC_RC"
  run "$TOOL" --matrix '1 0 0 0 -1 1'
  [ "$status" -eq 0 ]
  ! grep -q calibrationMatrix "$LABWC_RC"
  # siblings on the same line must survive (only the element is removed)
  grep -q '<labwc_config><core></core></labwc_config>' "$LABWC_RC"
  [ -e "$LABWC_RC.bak" ]
}

@test "unknown option is a usage error" {
  run "$TOOL" --bogus
  [ "$status" -eq 1 ]
}

@test "a non-numeric timeout is refused" {
  run "$TOOL" --matrix '1 0 0 0 -1 1' --timeout abc
  [ "$status" -eq 1 ]
}
