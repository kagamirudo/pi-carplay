# Quality policy

pi-carplay runs in a car. Treat a crash, a frozen screen or a lost phone connection as a defect that can distract a driver. This policy sets the rules every change and every release follows.

## Rules

1. **Nothing reaches `main` without review and green CI.** `main` is protected: changes land through pull requests, every gate in the [pipeline](#gates) must pass, the branch must be up to date with `main`, and history stays linear. Force pushes and branch deletion are blocked.
2. **Releases come only from `main`.** A release tag must point at a commit already on `main`, match `VERSION`, and have a `CHANGELOG.md` entry. The release workflow re-runs every gate on the tagged commit before it publishes anything.
3. **Every bug fix comes with a test** that fails before the fix and passes after it.
4. **Every user-facing change is covered** by a unit, UI or smoke test at the lowest level that can catch it (see [testing.md](testing.md)).
5. **Coverage only goes up.** `make coverage` fails below `COVERAGE_MIN` (Makefile). Raise it when coverage rises; never lower it to get a change through.
6. **No warnings.** The shell builds with `-Werror`. shellcheck, cppcheck and qmllint must be clean. A QML warning at run time fails the UI tests.
7. **Fail safe, never fail closed.** Bad input (a corrupt config, a missing service, an odd Bluetooth name, an unplugged phone) must leave the head unit usable. The rest of this page lists the guarantees the code and the tests hold to.
8. **Every release passes the hardware checklist** on a real Pi, display and phone before its tag is pushed.

## Robustness guarantees

Each guarantee has a test that enforces it.

| Guarantee | Enforced by |
| --- | --- |
| A corrupt, empty, binary, oversized or unreadable `headunit.json` never stops the shell from starting. It is moved to `headunit.json.corrupt`, the shell starts on defaults and the Setup page says so. | `tests/smoke.sh`, `tst_ui` |
| The config parser never crashes or recurses without bound, whatever the bytes: depth limit 16, size limit 64 KiB. | `tst_config`, `tst_config_fuzz` (50,000 mutations per run, 2,000,000 nightly) |
| Saving settings survives a power cut: write to a temp file, `fsync`, rename, `fsync` the directory. | `tst_config`, `tst_presenter` |
| If the shell or Android Auto exits or crashes, it is restarted, with back-off and a crash record. | `pi-carplay-supervise`, `tests/scripts/supervise.bats` |
| No Bluetooth address that isn't a strict `AA:BB:CC:DD:EE:FF` reaches a shell command. | `tst_bluetooth_list` |
| The pairing agent answers only the device being paired and rejects everything else. | `tst_bluetooth_agent` (over a private D-Bus) |
| Without BlueZ, every Bluetooth button is a harmless no-op. | `tst_bluetooth_list`, `tests/smoke.sh` |
| Every screen opens, every button is at least 32×32 px, on screen, and does what it says, on a 480×320 panel. | `tst_ui` |
| Thousands of random taps cause no QML error and no memory growth (8 MiB limit). | `tst_ui_stress` (3,000 taps per run, 100,000 nightly) |
| No memory errors, leaks or undefined behaviour in the shell. | `make test-asan` (ASan + UBSan, every test) |
| The `.deb` installs on a clean Bookworm, starts, sets up a user, purges completely and reinstalls. | `packaging/test-deb.sh --install` in CI |

## Gates

`.github/workflows/pipeline.yml` runs these for every pull request, every push to `main` and every release tag. The `CI passed` job is the required status check.

| Gate | Command | Fails on |
| --- | --- | --- |
| Lint | `make lint` | Any shellcheck finding |
| Static analysis | `make analyze` | cppcheck warnings (warning, performance, portability), qmllint findings |
| Build | `make build` | Any compiler warning (`-Werror`) |
| Tests | `make test` | Any unit, integration, UI, stress, smoke or bats test |
| Sanitizers | `make test-asan` | Any ASan, LSan or UBSan report |
| Coverage | `make coverage` | Line coverage below `COVERAGE_MIN` |
| Package | `make test-package` | Missing files, unrendered templates, unresolved libraries, lintian errors, failed start |
| Install | `packaging/test-deb.sh --install` | Failed install, start, setup, purge or reinstall on a clean system |

`.github/workflows/soak.yml` runs every night: fuzzing and the UI monkey test at high volume, in release and sanitizer builds. A soak failure is a release blocker until it is fixed.

Run every gate locally before pushing:

```sh
make deps-qa   # once
make check
```

## Release procedure

1. **Freeze.** Merge the last change for the release. Confirm the last `main` CI run and soak are green.
2. **Version.** In a pull request, set `VERSION`, move the `Unreleased` notes in `CHANGELOG.md` under the new version, and merge it.
3. **Hardware checklist.** Install the CI package for that commit on the reference Pi (download it from the run's artifacts) and complete the [hardware checklist](#hardware-checklist). Record the result in the release pull request.
4. **Tag.** `git tag -a vX.Y.Z -m "pi-carplay X.Y.Z" && git push origin vX.Y.Z`. The release workflow re-runs every gate and publishes the `.deb`, the GPL source and `SHA256SUMS`.
5. **Verify.** Download the release assets, check `sha256sum -c SHA256SUMS`, install on the Pi, and run `pi-carplay-selftest`.

### Rollback

Every release stays published. To roll a Pi back: `sudo apt install ./pi-carplay_<previous>_arm64.deb --allow-downgrades`. A release with a crash or safety defect is marked as a pre-release on GitHub with a note pointing to the fix.

## Hardware checklist

Run on the reference hardware: Raspberry Pi 5, 3.5" ILI9486/ADS7846 panel, a 5 V 5 A supply, an Android phone with a USB data cable, and the car's Bluetooth or a Bluetooth speaker.

| # | Check | Pass when |
| --- | --- | --- |
| 1 | `pi-carplay-selftest` | 0 failed |
| 2 | Cold boot | The shell appears within 60 s of power-on, without touching anything |
| 3 | Touch | Each header tab highlights under your finger, top-left and bottom-right corners included |
| 4 | Android Auto | Plug the phone in: projection appears within 20 s. Maps, media and calls work |
| 5 | Exit and resume | AA's Exit tile shows the shell; Home → Android Auto brings projection back |
| 6 | Unplug and replug | Projection ends cleanly; replugging starts it again without a restart |
| 7 | Bluetooth | Scan, pair the car (PIN 0000 or 1234 accepted automatically), audio plays there |
| 8 | Shell crash | `pkill -SEGV -x headunit`: the shell is back within 5 s, and `crashes.log` records it |
| 9 | Projection crash | `pkill -SEGV -x autoapp`: `autoapp` is running again within 5 s, and projection returns (after `pi-carplay-restart` or a replug if the phone does not reconnect by itself) |
| 10 | Corrupt settings | `echo junk > ~/.config/raspi-headunit/headunit.json`, restart: the shell starts and says settings were reset |
| 11 | Power cut | Pull power while on the Setup page after saving; on boot, the saved settings are intact |
| 12 | One-hour drive or bench run | No restarts in `crashes.log`, no under-voltage in `vcgencmd get_throttled` |

## Reporting a crash

Open an issue with the bug report template. Attach `~/.local/state/pi-carplay/crashes.log`, the matching `headunit.log` or `autoapp.log`, and the `pi-carplay-selftest` output.
