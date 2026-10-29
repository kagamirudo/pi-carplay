# Changelog

All notable changes to this project are documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

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
