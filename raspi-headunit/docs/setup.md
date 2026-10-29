# Setup

For the full system (Android Auto, udev, labwc, display overlay) use the root `Makefile` and [docs/installation.md](../../docs/installation.md). This page covers the shell on its own.

Target is a Raspberry Pi 5 on Debian 12 bookworm. Stay on Bookworm. Qt 6.4 from the Bookworm archive is the GUI toolkit; do not pull Trixie Qt.

Packages the shell needs:

- `qt6-base-dev`, `qt6-declarative-dev`, `qt6-declarative-dev-tools`
- `qml6-module-qtquick`, `qml6-module-qtquick-controls`, `qml6-module-qtquick-layouts`, `qml6-module-qtquick-templates`, `qml6-module-qtquick-window`, `qml6-module-qtqml`
- `qt6-wayland` so the binary can open on labwc

Build and test:

```sh
cd /home/kagamirudo/CarPlay
cmake -S raspi-headunit -B raspi-headunit/build
cmake --build raspi-headunit/build -j4
ctest --test-dir raspi-headunit/build --output-on-failure
QT_QPA_PLATFORM=offscreen ./raspi-headunit/build/headunit --smoke
```

Run on the desktop with `raspi-headunit/scripts/run-shell.sh`. Close the window to get the desktop back. Do not kill labwc to show the shell.

## Boot unit

`systemd/headunit.service` is a **user** unit. `ExecStart` points at `~/CarPlay/raspi-headunit/build/headunit` and waits for `graphical-session.target`, which is this Pi's labwc login.

It is not copied into `~/.config/systemd/user` and it is not enabled. Enabling it would put the shell on top of the desktop at the next graphical login. Do that only when you want the shell instead of the desktop.

Dash-cam files, when a recorder exists, go on a USB disk. Do not point `clipsDirectory` at the 29 GB OS card. `StorageManager` rejects a path whose mount source is the OS disk.
