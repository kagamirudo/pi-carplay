# Installation

This walks through a fresh install with the `Makefile`. Every step can be re-run safely: fetching, patching and installing skip work that is already done.

## 1. Prepare the Pi

- Flash **Raspberry Pi OS (64-bit) with desktop, Bookworm**. Stay on Bookworm: the build targets its Qt 6.4 and Qt 5 packages.
- Boot, finish first-run setup, and log in to the desktop. The default compositor is labwc.
- Attach the 3.5″ SPI hat to the 40-pin header and, optionally, Camera Module 3 to **CAM/DISP 1**.
- Use a **5 V / 5 A** supply. Weaker supplies cause undervoltage, a white panel and reboots (see [troubleshooting](troubleshooting.md)).

## 2. Get the code

```sh
git clone https://github.com/kagamirudo/pi-carplay.git ~/CarPlay
cd ~/CarPlay
```

The repo can live anywhere. Paths in the installed files are filled in from where you cloned it.

## 3. Install packages

```sh
make deps
```

This installs the compilers, Qt 6 (shell), Qt 5 Multimedia (OpenAuto), Boost, protobuf, libusb, OpenSSL, BlueZ tools, `pactl`, `wlr-randr` and `grim`.

## 4. Build

```sh
make build
make test
```

`make build` does the following:

| Step | Result |
| --- | --- |
| `h264bitstream` | Fetched at a pinned commit, built, installed to `/usr/local` |
| `aasdk` | Fetched at a pinned commit, `patches/aasdk.patch` applied, installed to `/usr/local` |
| `openauto` | Fetched at a pinned commit, `patches/openauto.patch` applied, `autoapp` built in `~/src/openauto/bin` |
| `headunit` | The Qt 6 shell built in `raspi-headunit/build/headunit` |

Sources go to `~/src` by default. Override with `make build SRC_DIR=/opt/src`. A full first build takes roughly 15–30 minutes on a Pi 5. Later builds only recompile what changed.

## 5. Install configuration

```sh
make install
```

| File | Purpose |
| --- | --- |
| `/etc/udev/rules.d/51-android-accessory.rules` | Lets the desktop user open the phone after it switches to accessory mode (`18d1:2d0x`) |
| `~/.config/labwc/rc.xml` | Touch calibration matrix for the ADS7846 and mouse emulation |
| `~/.config/labwc/autostart` | Starts the shell, then Android Auto on top of it, at login |
| `~/src/openauto/bin/openauto.ini` | OpenAuto settings: 800×480 stream, 80 px margin, DPI 130, touch on. An existing file is kept. |
| `~/.local/bin/aa-restart.sh` | Restarts the shell and Android Auto and re-plugs the phone |

Existing labwc files that differ are backed up to `*.bak` first.

## 6. Enable the display

```sh
make boot-config
sudo reboot
```

This appends the overlay below to `/boot/firmware/config.txt`, unless a `piscreen` line already exists. The file is backed up to `config.txt.bak` first.

```ini
dtoverlay=piscreen,drm,speed=32000000,rotate=180,invx,invy
```

| Option | Why |
| --- | --- |
| `drm` | Uses the modern `ili9486` DRM driver, so labwc sees the panel as a normal output (`SPI-1`) |
| `speed=32000000` | The panel's rated maximum. See [display notes](troubleshooting.md#spi-speed) |
| `rotate=180,invx,invy` | Picture orientation and matching raw touch axes for this hat |

## 7. First run

After the reboot the panel shows the shell's Home page. Plug in the phone:

1. The phone asks to allow Android Auto. Accept it. The first run may also ask you to update or install Android Auto.
2. The phone re-enumerates in accessory mode, and `autoapp` starts the session full-screen.
3. Tap the screen to confirm touch lines up.

If touch is mirrored on your hat, edit `deploy/labwc/rc.xml` (`calibrationMatrix`) and run `make install-labwc`. Common matrices:

| Effect | Matrix |
| --- | --- |
| As shipped (vertical flip) | `1 0 0 0 -1 1` |
| None | `1 0 0 0 1 0` |
| Rotate 180° | `-1 0 1 0 -1 1` |
| Horizontal flip | `-1 0 1 0 1 0` |

## Updating

```sh
git pull
make build install
make restart
```
