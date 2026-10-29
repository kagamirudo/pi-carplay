# Testing

All tests run on any arm64 or x86-64 Linux with the build dependencies installed; no Pi, display, phone or Bluetooth adapter is needed. CI runs them on Debian Bookworm arm64.

```sh
make deps deps-qa   # once
make test           # build the shell, run every ctest test and the bats tests
make check          # everything CI checks: lint, analysis, tests, sanitizers, coverage
```

## Layout

| Where | What |
| --- | --- |
| `raspi-headunit/tests/tst_*.cpp` | Qt Test executables, one per area, registered with ctest |
| `raspi-headunit/tests/UiFixture.hpp` | Loads the real QML with the real presenter and a fake Bluetooth backend |
| `raspi-headunit/tests/smoke.sh` | Starts the real `headunit` binary under good and broken config states |
| `raspi-headunit/tests/panel-480x320.json` | Offscreen screen matching the panel, so layouts are tested at the real size |
| `tests/scripts/*.bats` | Tests for the supervisor, launcher, setup, boot-config, wireless and update-check scripts. `wireless.bats` uses a fake sysfs and stub `iw`, `nmcli`, `ss` and `sudo`, so it needs no radio; `update.bats` runs `pi-carplay-update` against a local fake of the GitHub API (`fake_github.py`) |
| `packaging/test-deb.sh` | Checks for the built package; `--install` exercises it on a clean system |
| `tools/pi-carplay-selftest` | On-device check of a real head unit |
| `tools/pi-carplay-wireless-check` | On-device check of Bluetooth adapters, the 5 GHz hotspot and ports for wireless Android Auto |

## ctest labels

| Label | Tests | Typical time |
| --- | --- | --- |
| `unit` | config, storage, phone, application, presenter, runtime, Bluetooth list | seconds |
| `fuzz` | config parser mutation fuzzing | under a second |
| `integration` | the pairing agent over a private D-Bus session | under a second |
| `ui` | every page, every button, layout limits, the Bluetooth list | a few seconds |
| `stress` | seeded random taps with memory tracking | about 10 s |
| `smoke` | the real binary under 13 environment and config cases | a few seconds |

```sh
ctest --test-dir raspi-headunit/build -L unit          # one label
ctest --test-dir raspi-headunit/build -R presenter -V  # one test, verbose
raspi-headunit/build/tests/tst_ui settingsSaveFromTheScreen   # one test function
```

## Test environment

ctest sets these for every test, so tests can never touch the real system:

| Variable | Effect |
| --- | --- |
| `QT_QPA_PLATFORM=offscreen:configfile=panel-480x320.json` | No display needed; the screen is 480×320 like the panel |
| `HEADUNIT_NO_BLUETOOTH=1` | The shell never talks to BlueZ or `bluetoothctl` |

The Android Auto button runs an injected command in tests (`ShellPresenter::setResumeCommand`) instead of signalling the real `autoapp`. The pairing agent test runs under `dbus-run-session` with a private bus.

## Volume knobs

| Variable | Default | Used by |
| --- | --- | --- |
| `HEADUNIT_FUZZ_ITERATIONS` | 50,000 | `tst_config_fuzz` |
| `HEADUNIT_STRESS_TAPS` | 3,000 | `tst_ui_stress` |
| `HEADUNIT_STRESS_MAX_GROWTH_KIB` | 8192 | `tst_ui_stress` memory limit (skipped under ASan) |

The nightly soak workflow runs 2,000,000 fuzz iterations and 100,000 taps.

## Sanitizers and coverage

```sh
make test-asan    # build-asan/: ASan + UBSan, leak checking at exit
make coverage     # build-cov/: gcovr summary, coverage.html, fails below COVERAGE_MIN
```

Open `raspi-headunit/build-cov/coverage.html` for line-by-line coverage.

## Writing a test

- Pick the lowest level that catches the problem: pure logic → a `unit` test; something on screen → `tst_ui`; start-up or environment → `smoke.sh`; a script → bats.
- Add a test function to the matching `tst_*.cpp`, or a new file plus one `headunit_test(...)` line in `tests/CMakeLists.txt`.
- Put JSON or other raw-string fixtures in a header such as `ConfigFixtures.hpp`: Qt's moc cannot parse raw string literals in a test source.
- Tests must be deterministic: seed random generators, use `QTRY_*` instead of fixed sleeps, and create files only under `QTemporaryDir`.
