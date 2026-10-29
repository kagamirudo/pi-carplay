# CI and releases

The rules behind all of this, and the hardware checklist every release passes, are in [quality.md](quality.md).

## Workflows

| Workflow | Runs on | What it does |
| --- | --- | --- |
| `ci.yml` | Every pull request, push to `main`, and on demand | Runs the pipeline |
| `pipeline.yml` | Called by CI and Release | Every quality gate (below) |
| `build.yml` | Called by the pipeline | Full build, analysis, tests, `.deb`, package checks |
| `release.yml` | Tags `v*` | Tag checks, the pipeline, then publishes the release |
| `soak.yml` | Nightly, and on demand | High-volume fuzzing and UI monkey tests, release and sanitizer builds |

### Pipeline jobs

| Job | Runner | What it does |
| --- | --- | --- |
| Lint | `ubuntu-latest` | `make lint` (shellcheck) and a `make -n` dry run |
| Build and test | `ubuntu-24.04-arm`, `debian:bookworm` | `make deps deps-qa build analyze test package test-package`; uploads the `.deb`, GPL source and JUnit results |
| ASan + UBSan | `ubuntu-24.04-arm`, `debian:bookworm` | `make test-asan` |
| Coverage | `ubuntu-24.04-arm`, `debian:bookworm` | `make coverage`; uploads the HTML report |
| Install test | Fresh `debian:bookworm` | `packaging/test-deb.sh --install` on the built `.deb` |
| CI passed | `ubuntu-latest` | Fails unless every job above succeeded. Branch protection requires this check |

Builds run on native arm64 Debian Bookworm, the same base as Raspberry Pi OS, so a CI package installs on a Pi as-is. CI builds are versioned `<VERSION>~git<sha>`, which sorts below the matching release.

## Cutting a release

Follow the [release procedure](quality.md#release-procedure). In short:

1. In a pull request, set `VERSION` (for example `0.2.0`) and move the `Unreleased` notes in `CHANGELOG.md` under `## [0.2.0] - YYYY-MM-DD`. Merge it once CI passes.
2. Complete the [hardware checklist](quality.md#hardware-checklist) with the CI package of that commit.
3. Tag the merged commit and push the tag:

   ```sh
   git switch main && git pull
   git tag -a v0.2.0 -m "pi-carplay 0.2.0"
   git push origin v0.2.0
   ```

`release.yml` then runs:

| Job | What it does |
| --- | --- |
| Check tag | Fails if the tag doesn't match `VERSION`, `CHANGELOG.md` has no entry for it, or the commit is not on `main` |
| Gates | The full pipeline on the tagged commit |
| Publish | Creates the GitHub release from the changelog entry, with install instructions |

The release attaches:

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
| `/usr/bin/pi-carplay-supervise` | Keeps the shell and Android Auto running; records crashes |
| `/usr/bin/pi-carplay-autoapp` | Runs `autoapp` from `~/.config/pi-carplay/openauto` |
| `/usr/bin/pi-carplay-restart` | Restarts both programs and resets the phone's USB port |
| `/usr/bin/pi-carplay-boot-config` | Adds the SPI display overlay to `config.txt` |
| `/usr/bin/pi-carplay-selftest` | On-device health check |
| `/lib/udev/rules.d/51-android-accessory.rules` | Accessory-mode USB access for the desktop user |

Runtime dependencies are worked out at build time from the libraries the shipped binaries link directly, plus the QML modules, Qt Wayland, Qt Multimedia plugins, BlueZ and `pactl`.

## Building and checking a package locally

```sh
make build
make package                   # dist/pi-carplay_$(cat VERSION)_arm64.deb
make test-package              # static checks, lintian, smoke start from the package tree
make package VERSION=0.2.0~rc1
```

`packaging/test-deb.sh --install` changes the system it runs on. Run it only in a throwaway container or VM.
