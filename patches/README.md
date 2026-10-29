# Patches to third-party projects

These patches modify GPL-3.0 software. They are licensed under the **GNU General Public License v3.0**, the same as the projects they apply to. They are not covered by this repository's MIT license.

| Patch | Upstream | Base commit |
| --- | --- | --- |
| `aasdk.patch` | [openDsh/aasdk](https://github.com/openDsh/aasdk) (`develop`) | `1bc0fe69d5f5f505c978a0c6e32c860e820fa8f6` |
| `openauto.patch` | [openDsh/openauto](https://github.com/openDsh/openauto) (`develop`) | `e7caeb4d49186af867c1d4693c9f3bf9e9ef99e0` |

`make aasdk` and `make openauto` fetch these exact commits and apply the patches. They skip a patch that is already applied.

## aasdk.patch

- **OpenSSL 3:** `SSLWrapper` no longer calls `FIPS_mode_set` and the other teardown functions that OpenSSL 3 removed. Debian 12 ships OpenSSL 3.
- **Accessory open retry:** `USBHub` retries opening the phone after it re-enumerates in accessory mode (`18d1:2d00`). It tries 25 times, 200 ms apart, until udev has applied the permission rule. Other USB devices that fail to open are skipped quietly.

## openauto.patch

- **Video on a small panel:** `QtVideoOutput` uses a `QGraphicsView` instead of `QVideoWidget`.
  - It asks the phone for 800×480 with an 80 px width margin, so Android Auto lays out a 720×480 (3:2) content area.
  - It crops that area and scales it evenly by 2/3 onto the 480×320 panel, with smooth scaling.
- **Touch mapping:** `InputDevice` maps panel touches into the 720×480 content area and reports that geometry to the phone. Taps within 14 px count as taps, not drags.
- **Return to the head unit:** Android Auto's **Exit** tile sends a video-focus request with mode `UNFOCUSED`.
  - `VideoService` now answers it and hides the projection window.
  - `SIGUSR1` shows the window again and sends an unrequested `FOCUSED` indication, so the phone resumes video. The shell's Android Auto button sends this signal.
- **`OPENAUTO_NO_MAIN_WINDOW=1`:** hides OpenAuto's own launcher window when a separate shell owns the screen.
- **Optional colour correction:** a per-pixel gamma, contrast and saturation effect. It is off unless `OPENAUTO_PANEL_GAMMA`, `OPENAUTO_PANEL_CONTRAST` or `OPENAUTO_PANEL_SATURATION` is set to something other than `1.0`.

## Regenerating

After editing the sources in `$SRC_DIR` (default `~/src`):

```sh
git -C ~/src/aasdk diff > patches/aasdk.patch
git -C ~/src/openauto diff > patches/openauto.patch
```
