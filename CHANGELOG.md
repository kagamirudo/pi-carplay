# Changelog

All notable changes to this project are documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- **Dashcam:** `pi-carplay-dashcam` records Camera Module 3 to the SD footage volume as one-minute MP4s, drops the oldest finished clip to stay under the byte budget, and shows a Qt preview on the Cam tab when the Wayland session is up. The systemd unit is installed by `make install` and is not enabled by the package.
- **Display profiles:** `pi-carplay-display-profile` keeps the Dell HDMI profile until `use elecrow-7` is run for the 1024×600 panel. `pi-carplay-align-display` reads the active profile.
- **Elecrow housing:** `hardware/elecrow-7/housing.py` builds the bezel, rear shell, camera bracket and fit gauge for the 7-inch module and a Pi 5.
- **Crash recovery:** `pi-carplay-supervise` runs the shell and Android Auto, restarts either one if it exits (back-off from 1 s to 30 s), records each crash in `~/.local/state/pi-carplay/crashes.log`, and rotates the logs. Autostart and `pi-carplay-restart` use it.
- **`pi-carplay-selftest`** (and `make selftest`): on-device check of power, temperature, display, touch, Bluetooth, audio, udev rule, autostart and recent crashes.
- **Wireless Android Auto preparation** ([docs/wireless.md](docs/wireless.md)):
  - which TP-Link Bluetooth adapters work (UB500 / UB500 Plus recommended, UB600 not supported by the Bookworm kernel)
  - what the protocol needs, and the roadmap
  - `pi-carplay-wireless-check` (`make wireless-check`): read-only check of Bluetooth controllers and firmware, 5 GHz access point support, regulatory country, NetworkManager and the projection ports
  - `pi-carplay-wireless-setup` (`make wireless-setup`): installs `firmware-realtek`, records the phone's Bluetooth adapter and writes a 5 GHz hotspot profile with autoconnect off. It has `--dry-run`, is safe to run again, and never starts the hotspot.
  - wireless projection itself is not available yet
- **Test suite:** Qt Test unit, D-Bus integration, UI and smoke tests, a seeded UI monkey test, config fuzzing, and bats tests for the scripts. `make check` runs every gate.
- **Quality pipeline:** every pull request, push and release runs lint, static analysis, `-Werror` builds, all tests, ASan/UBSan, a coverage floor and a clean-system install test of the `.deb`. Nightly soak workflow.
- **`pi-carplay-update check`:** asks GitHub Releases whether a newer package exists (text or `--json`; exit 10 when there is one). Read-only; the over-the-air update design is in `docs/plans/0002-ota-updates.md`.
- Quality policy with the release procedure and hardware checklist, a testing guide, contributing guide, and pull request and bug report templates.
- README screenshots of Android Auto on the 480×320 panel (dashboard and full-screen Maps), next to the head-unit shell, with map and personal details pixelated.

### Changed

- `plan_01.md` moved to [`docs/plans/0001-pi5-head-unit-and-dash-cam.md`](docs/plans/0001-pi5-head-unit-and-dash-cam.md), with a status header and a [plans index](docs/plans/README.md) for future design plans.
- Logs moved from `/tmp` to `~/.local/state/pi-carplay/`, so they survive a reboot.
- The Android accessory udev rule grants access to the logged-in user and `plugdev` (`0660`, `uaccess`) instead of every user (`0666`).

### Fixed

- A corrupt, empty or unreadable `headunit.json` no longer stops the shell from starting. It is moved aside, the shell starts on defaults and the Setup page says so.
- Deeply nested JSON in the config could overflow the stack; nesting and file size are now limited.
- Saved settings could be lost if power was cut right after saving; the file is now flushed to disk before it replaces the old one.
- Tapping the current page's tab un-highlighted it and broke the highlight for later taps.
- Bluetooth list rows with missing fields produced QML errors.
- The pairing agent could crash if a rejection happened outside a D-Bus call.
- Bundled libraries in the `.deb` were marked executable; `postinst` ran `ldconfig` needlessly.

## [0.1.0] - 2026-10-29

First release: a working wired Android Auto head unit on a Raspberry Pi 5 with a 3.5″ SPI touchscreen.

### Added

- **Wired Android Auto** using OpenAuto and aasdk, patched for this panel:
  - the phone lays out 720×480, scaled ×2/3 onto 480×320
  - touch mapped into the content area
  - OpenSSL 3 support
  - accessory-mode open retry
- **Exit and back:** Android Auto's Exit tile returns to the shell, and the shell's Android Auto button resumes projection.
- **Head-unit shell** (Qt 6 / QML) with Home, Phone, Cam, BT and Setup pages.
- **Bluetooth device list:**
  - scan, pair, connect, disconnect and forget
  - built-in pairing agent that answers PIN `0000` and then `1234`, and confirms numeric comparison
  - audio routed to the connected device through PipeWire
- **Login autostart:** shell underneath, Android Auto on top.
- **`aa-restart.sh` / `pi-carplay-restart`** to recover a stalled session.
- **Makefile** for dependencies, build, test, install, boot config and packaging.
- **`.deb` package** for Raspberry Pi OS Bookworm arm64 with bundled OpenAuto and aasdk, plus a GPL source tarball.
- **CI and release workflows** on GitHub Actions.

[Unreleased]: https://github.com/kagamirudo/pi-carplay/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/kagamirudo/pi-carplay/releases/tag/v0.1.0
