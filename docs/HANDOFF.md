# Handoff notes (2026-11-01)

Context for the next agent. Verified on hardware unless noted.

## Access
- Pi 5, Bookworm, kernel 6.12 aarch64. `ssh kagami-board` (= `kagamirudo@192.168.2.2`), repo at `~/CarPlay` with `~/CarPlay-*` worktrees per branch.
- The cable (`192.168.2.2`) only works while macOS **Internet Sharing** (Wi-Fi → AX88179B) is on. SSH timeout → check that first. Guest Wi-Fi is client-isolated; Tailscale is on the Pi only.
- No physical panel attached → labwc runs a headless 1920×1080 output. View via `vnc://192.168.2.2:5900` (transient `wayvnc`, not persistent) or `grim`. Camera stills need `rpicam-still --autofocus-range macro`.

## Biggest finding: the SPI panel is bad hardware
The 3.5" ILI9486 panel renders correct framebuffer pixels (grim is vivid) but the glass washes blue. Solid-colour test straight to `/dev/fb0`: red→red, blue→blue, but **black→blue-grey and white→cyan**. Primaries fine ⇒ not a driver/format bug; the panel can't make black/white (dead TN contrast). VCOM and inversion changed nothing (tested live). **Fix = an IPS panel** (listing must say IPS; "TFT" at this size is TN). Prefer HDMI/DSI IPS to skip the SPI driver. An `hardware/elecrow-7/` housing + `deploy/display/profiles/elecrow-7.env` already target a 1024×600 Elecrow HDMI panel.

## Untracked state on the Pi (clean these up)
These are live changes NOT in git and will not survive updates:
1. Hand-built `ili9486.ko` with sysfs `vcom`/`inversion` knobs + unconditional reset. Stock backup: `…/tiny/ili9486.ko.xz.orig`. Restore + `depmod -a` + `update-initramfs -u -k $(uname -r)` + reboot. **Gotcha:** the module loads from the initramfs, so swapping the `.ko` alone does nothing — rebuild the initramfs for the *running* kernel.
2. Boot trimmed 21 s → 9 s via `systemctl disable` of wayvnc / NetworkManager-wait-online / cups / ModemManager / rpi-eeprom-update + splash off + `tailscaled-late.timer`. Re-enabling wayvnc costs ~9 s.

## Power
Was undervolting on a 5.2 V/2.4 A supply. Now on 5 V/3 A, clean. At 3 A the USB ports share 600 mA — get a 5 V/5 A supply before adding a USB SSD or Carlinkit dongle. Pi 5 has no HW H.264 codec, so AA decode + dashcam encode are CPU-bound.

## PR status (branch protection: strict up-to-date + "Gates / CI passed" required; enforce_admins on; squash-only)
Merged this session: #5–#11, #13, #15, #16, #19. Open: **#12** (Display colour tab) — mid-merge, see below. The strict gate forces sequential rebase-then-merge; owner can disable with:
`gh api -X PATCH repos/kagamirudo/pi-carplay/branches/main/protection/required_status_checks -F strict=false -f 'contexts[]=Gates / CI passed'`

### #19 folded the connectivity UI into main
Dropdown page selector (not tab row); Wi-Fi tab (saved networks only — no password typing without a keyboard; new nets added over SSH); **Car mode** toggle on the BT tab that launches Android Auto, hides the Wi-Fi tab and runs the `pi-carplay-aa` hotspot; Home **Reconnect phone** button; `aa-restart.sh` now restarts autoapp + replugs USB **only** (not the shell); Android Auto projection follows the active `deploy/display/profiles/*.env` (AA_RES/AA_DPI/AA_MARGIN_*). It superseded #14/#17/#18 (closed).

### #12 — the one still open
Adds Settings→Display brightness/contrast/hue/saturation. **Shell-only** (does not touch Android Auto projection) so it does NOT fix the panel washout — the IPS panel does. Merged carefully onto #19's `Main.qml`: the page `StackLayout` is wrapped in `Qt5Compat.GraphicalEffects` `HueSaturation`+`BrightnessContrast` layers whose `layer.enabled` is false unless a slider is non-zero, so the default path is unchanged. Needs runtime dep `qml6-module-qt5compat-graphicaleffects` (in Makefile `APT_PACKAGES` + the .deb Depends), and `make analyze` passes `--import disable` to qmllint because Debian ships that module without `.qmltypes`.

## Repo gotchas learned
- JSON config parser (`ConfigurationManager`) rejects fractions and negatives in `readUint` → store small signed values as ints (e.g. display adjust is −100..100, QML divides).
- `make analyze` runs cppcheck **and** qmllint; qmllint fails `make analyze` on unknown imports.
- Coverage gate is 80% of `src/` (excludes `main.cpp`); wrap untestable process-I/O in `GCOVR_EXCL_START/STOP` (done for `WifiDevices` nmcli glue).
- Codex reviews post inline comments; branch protection requires resolving them before merge.
- UI tests drive the dropdown via the window's `allPages` property (full list), so hiding a tab (car mode) doesn't break navigation.

## Where I was built from
This session often built on the Mac (`~/pi-carplay-housing` clone) when the Pi cable dropped — `manifold3d`/Qt can't both build there; the Pi is the real build host. The `hardware/elecrow-7/housing.py` (manifold3d) parts were fixed so every part is one connected solid (verified by mesh shell-count); still needs a physical fit-test with the real module.
