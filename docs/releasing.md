# CI and releases

## Continuous integration

`.github/workflows/ci.yml` runs on every push to `main`, every pull request, and on demand.

| Job | Runner | What it does |
| --- | --- | --- |
| Lint | `ubuntu-latest` | `shellcheck` on all scripts (`make lint`) and a `make -n` dry run |
| Build | `ubuntu-24.04-arm` in a `debian:bookworm` container | `make deps build test package`, then uploads the `.deb` and GPL source tarball as a workflow artifact |

The build runs on native arm64 Debian Bookworm, the same base as Raspberry Pi OS. The package from CI installs on a Pi as-is. CI builds are versioned `<VERSION>~git<sha>`, which sorts below the matching release.

The build job lives in `.github/workflows/build.yml`, a reusable workflow, so CI and releases build exactly the same way.

## Cutting a release

1. Update `VERSION` (for example `0.2.0`). CMake reads the shell's version from the same file.
2. Move the `Unreleased` notes in `CHANGELOG.md` under a new `## [0.2.0] - YYYY-MM-DD` heading, and update the compare links.
3. Commit, then tag and push:

   ```sh
   git commit -am "Release 0.2.0"
   git tag -a v0.2.0 -m "pi-carplay 0.2.0"
   git push origin main v0.2.0
   ```

`.github/workflows/release.yml` then runs these jobs:

| Job | What it does |
| --- | --- |
| Check tag | Fails if the tag doesn't match `VERSION` or `CHANGELOG.md` has no entry for it |
| Build | Same reusable build as CI, versioned from the tag |
| Publish | Creates the GitHub release |

The release notes come from the changelog entry plus install instructions. The release attaches:

| Asset | Contents |
| --- | --- |
| `pi-carplay_<version>_arm64.deb` | The package |
| `pi-carplay-<version>-gpl-source.tar.gz` | Corresponding source for the GPL-3.0 OpenAuto and aasdk binaries, and the LGPL h264bitstream, with patches applied |
| `SHA256SUMS` | Checksums |

## What the package contains

| Path | Contents |
| --- | --- |
| `/opt/pi-carplay/bin/headunit` | The shell |
| `/opt/pi-carplay/bin/autoapp` | OpenAuto, `RUNPATH=$ORIGIN/../lib` |
| `/opt/pi-carplay/lib/` | Bundled `libopenauto`, `libbtservice_proto`, `libaasdk`, `libaasdk_proto` |
| `/opt/pi-carplay/share/` | labwc `rc.xml` and `autostart` templates, default `openauto.ini` |
| `/usr/bin/pi-carplay-setup` | Per-user labwc setup. `--boot` also adds the display overlay |
| `/usr/bin/pi-carplay-autoapp` | Runs `autoapp` from `~/.config/pi-carplay/openauto` |
| `/usr/bin/pi-carplay-restart` | Restarts both programs and resets the phone's USB port |
| `/usr/bin/pi-carplay-boot-config` | Adds the SPI display overlay to `config.txt` |
| `/lib/udev/rules.d/51-android-accessory.rules` | Accessory-mode USB permissions |

Runtime dependencies are worked out at build time from the libraries the shipped binaries link directly, plus the QML modules, Qt Wayland, Qt Multimedia plugins, BlueZ and `pactl`.

## Building a package locally

```sh
make build
make package                   # dist/pi-carplay_$(cat VERSION)_arm64.deb
make package VERSION=0.2.0~rc1
```
