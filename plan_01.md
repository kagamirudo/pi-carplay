# Project: Raspberry Pi 5 — DIY CarPlay, Android Auto, and Dash Cam

## 1. Project Overview

Build a small, self-contained infotainment unit on the Raspberry Pi 5 already on this bench. It should show Apple CarPlay and Android Auto, send audio to the car's existing Bluetooth stereo, and record the road ahead with the camera that is already attached.

The vehicle's original stereo stays as it is. The Pi only needs power.

### Core architecture

**Phone (iPhone or Android) → Raspberry Pi 5 → car Bluetooth stereo**

**Camera Module 3 on the back of the unit → local loop recording**

The phone connects wired (USB) or wireless (Wi-Fi, with Bluetooth used for discovery where the protocol requires it). The Pi:

1. Receives and displays Apple CarPlay or Android Auto.
2. Renders that interface on the attached display, facing the driver.
3. Forwards touch input to the phone.
4. Routes phone audio to the car stereo over Bluetooth A2DP.
5. Reconnects known phones and the car stereo on its own.
6. Records a forward dash-cam view from the camera on the opposite face of the unit.
7. Keeps recording across reboots with a loop that overwrites the oldest clips.

CarPlay stays a later milestone. Android Auto, the display, and the dash cam are the path to a useful prototype.

### How it sits in the car

Mount the finished unit at the rearview mirror.

- The display faces the driver.
- The back of the unit faces the mirror and the windshield.
- The camera is mounted on that back face and looks forward through the glass. That is the dash cam.
- The printed housing is a later step. It is not part of the first software bring-up. Once the display model and the real stack height are known, fetch or draw a printable enclosure that matches this orientation.

## 2. What is on this board

Checked on this machine (`kagamirudo`, Debian 12 bookworm, kernel `6.12.109+rpt-rpi-2712`). These are measurements, not the original shopping list.

| Item | Detected |
| --- | --- |
| Board | Raspberry Pi 5 Model B Rev 1.0, 4 GB RAM, Cortex-A76 @ 2.4 GHz |
| OS disk | 29 GB microSD (`mmcblk0`). About 9 GB free on `/`. Too small to hold both the OS and dash-cam footage. |
| Camera | Raspberry Pi Camera Module 3, standard lens. Sony IMX708, 4608×2592, autofocus (DW9817 VCM). Module id `0x0301`. libcamera tuning file `imx708.json` (not the wide or NoIR profiles). Plugged into **CAM/DISP 1** (`csi@128000`, `cam1_reg` on). Still image/video open works. |
| Free MIPI port | **CAM/DISP 0** is unused. The display is not on DSI or HDMI. |
| Display | 3.5 inch SPI hat, brought up. DRM `ili9486` on `spi0.0`, mode **480×320**, framebuffer `/dev/fb0` (`ili9486drmfb`). Touch is `ADS7846 Touchscreen` on `spi0.1`. Backlight is on. |
| Cooler | Official PWM fan is present and spinning (about 4100 RPM at ~55 °C). The case has to leave that airflow clear. |
| Radios | Onboard Wi-Fi and Bluetooth exist. `wlan0` is down. Bluetooth is powered off. Ethernet is up. |
| Toolchain already installed | CMake, BlueZ, PipeWire, WirePlumber, GStreamer 1.22, libcamera / rpicam-apps, Qt 6.4 core and QML (Bookworm packages: `qt6-base-dev`, `qt6-declarative-dev`, Quick Controls, `qt6-wayland`). |

The original plan assumed a 7–8 inch screen. This hat is 480×320. A test image (color bars and the word CARPLAY) was written to `/dev/fb0` and the pixels read back correctly. `/boot/firmware/config.txt` contains `dtoverlay=piscreen,drm,rotate=180,invx,invy`. After boot the DRM connector is `SPI-2` on card3, 480×320, and labwc is using it.

### Display pinout

Probed on the header, then checked against the in-tree overlay.

- SPI0 chip-select 0 (GPIO8): LCD. It does not drive MISO, which is normal for these hats.
- SPI0 chip-select 1 (GPIO7): touch controller. It answers ADS7846/XPT2046 conversion commands. With nothing pressed, Z1 stays at 0 and Y/Z2 sit at full scale.
- GPIO17: touch IRQ, held high by the hat (pen up). It stays high even against the Pi's pulldown.
- The other header GPIOs float, so the hat is not a DPI panel and it has no I2C touch chip.

That is the PiScreen / Waveshare 3.5 inch RPi LCD pinout. The kernel overlay is `piscreen` with the `drm` parameter (`waveshare,rpi-lcd-35`, ILI9486 driver, ADS7846 touch).

The driver is in the Raspberry Pi kernel tree, same file as mainline:

- https://github.com/raspberrypi/linux/blob/rpi-6.12.y/drivers/gpu/drm/tiny/ili9486.c
- https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/tiny/ili9486.c
- Overlay: https://github.com/raspberrypi/linux/blob/rpi-6.12.y/arch/arm/boot/dts/overlays/piscreen-overlay.dts

Author of the DRM driver is Kamlesh Gurudasani. It is a TinyDRM panel, 480×320, about 73×49 mm. This hat has an SPI-to-16-bit parallel converter in front of the ILI9486, so the driver sends each command byte as a 16-bit SPI word (`waveshare_command` in that file). A plain 8-bit ILI9341 init will not drive it. Pixel format is RGB565 (`0x55`). Rotation 0 sets MADCTL to MV|MY|MX plus BGR.

| Signal | BCM GPIO | Header pin |
| --- | --- | --- |
| LCD chip-select | GPIO8 (SPI0 CE0) | 24 |
| Touch chip-select | GPIO7 (SPI0 CE1) | 26 |
| MOSI / MISO / SCLK | GPIO10 / GPIO9 / GPIO11 | 19 / 21 / 23 |
| Data/command | GPIO24 | 18 |
| Reset | GPIO25 | 22 |
| Backlight | GPIO22 | 15 |
| Touch IRQ | GPIO17 | 11 |

The stock runtime `dtoverlay=piscreen,drm` binds touch immediately, then the panel waits forever on the `piscreen_pins` pin group. That group is only created when the overlay is applied from `config.txt` at boot. A runtime overlay with the same GPIOs and no pin group did bind: `ili9486` minor 2, connector `card2-SPI-1`, mode 480×320, backlight on.

## 3. Hardware

### In hand

- Raspberry Pi 5 Model B, 4 GB
- Raspberry Pi Camera Module 3 (standard) on CAM/DISP 1, with its CSI cable
- SPI touchscreen hat on the 40-pin header (Waveshare 3.5 inch / PiScreen pinout, ILI9486-class panel, ADS7846 touch)
- Official active cooler
- ~32 GB microSD (OS only)
- Onboard Wi-Fi and Bluetooth

### Still to pin down

- On the next reboot, confirm `dtoverlay=piscreen,drm,rotate=180,invx,invy` still produces the 480×320 connector, and that a tap lands on the pixel that was touched. The panel and the driver are already proven in this session.
- A USB SSD or a much larger card for dash-cam clips. Loop recording does not go on the OS card.
- Regulated 5 V / 5 A supply for the car. In the car this also needs a clean shutdown path so a clip is not left half-written when ignition power drops.
- Optional second Bluetooth adapter only if one radio cannot be the A2DP source to the car and the phone-discovery radio at the same time.
- Optional USB microphone for calls. The Camera Module 3 has no microphone.
- Apple MFi authentication hardware, only when CarPlay work actually starts.

### Housing (later)

A custom printed enclosure, drawn after the hat is confirmed on screen and its outline is measured.

Design constraints to carry forward:

- Driver face: display, touchable, readable in daylight.
- Opposite face: Camera Module 3 lens, aimed through the windshield, with a clear cone in front of the lens and a path for the CSI cable back to CAM/DISP 1.
- The unit's back faces the rearview mirror. The lens looks past the mirror area out the glass, not into the mirror housing.
- Clearance and vents for the active cooler. A sealed box on a sunny dash will throttle a Pi 5 that is decoding phone video and encoding the dash cam together.
- The screen is a hat on the 40-pin header, not a separate DSI ribbon. The print has to clear that stack and the active cooler on the same side of the board. The camera still leaves through the CSI cable to the back face.
- Mount that grips the mirror or a mirror-stem bracket, and does not block the driver's view of the road.

No STL in this revision. After the first successful boot of the hat, measure the stack and either search for a close printable shell or generate one.

## 4. Technology stack

- **OS:** Raspberry Pi OS / Debian 12, 64-bit (already installed)
- **Language:** C++20
- **GUI:** Qt 6 + QML, sized to the real panel
- **Phone video:** GStreamer, hardware-accelerated where the chosen receiver allows it
- **Dash cam:** libcamera / the PiSP pipeline already on this board, hardware H.264 or HEVC encode, segmented files
- **Audio:** PipeWire + WirePlumber
- **Bluetooth:** BlueZ
- **Networking:** NetworkManager, plus Wi-Fi Direct or an access point if the wireless projection stack needs it
- **Touch:** ADS7846 on SPI0 CE1, IRQ GPIO17, into Qt as evdev (`/dev/input/event*`)
- **Services:** systemd
- **Build:** CMake
- **Version control:** Git

### Protocol code

Evaluate, and verify license plus current Pi 5 build status, before depending on any of these:

- LIVI: https://github.com/f-io/LIVI
- pi-carplay: https://github.com/cedmoreau/pi-carplay

Use them as references. Do not assume they drop into this tree.

## 5. Functional requirements

### A. Phone connection

Support Android and iOS.

**Wired:** USB data, hot-plug, reconnect after unplug.

**Wireless:** discovery, the Wi-Fi link the protocol expects, automatic session start, reconnect to known phones, and a clean failure path.

Wireless is the eventual default. USB stays as the fallback.

### B. Android Auto

- Wired first, wireless second.
- Decode the video stream onto the small display.
- Forward touch.
- Route audio into PipeWire, then to the car.
- Surface connection state.

Prefer an existing open-source receiver where the license and the technical fit are both acceptable.

### C. Apple CarPlay

- Wired and wireless only if matching MFi hardware and a legal receiver stack exist.
- Video, touch, and audio follow the same routing as Android Auto.
- Do not bypass MFi authentication.

CarPlay is not required for the first prototype.

### D. Bluetooth audio to the car

The Pi is the A2DP source. The car stereo is the speaker.

1. Discover and pair the car's Bluetooth receiver.
2. Reconnect when the car is present.
3. Take audio from the projection session and from local UI sounds.
4. Route it in PipeWire.
5. Send it as A2DP.

Calls are a separate problem. A2DP does not carry the phone call. HFP, a microphone, and echo handling have to be proven before call support is claimed. The dash cam does not provide that microphone.

### E. Display and touch

The UI is fullscreen on whatever small panel is actually attached.

Show:

- Connection and device list
- Wired versus wireless state
- Car-stereo Bluetooth pairing and audio output
- Dash-cam state: recording, storage free, last clip
- Basic settings

Launch at boot. When CarPlay or Android Auto is active, that image owns the screen. Dash-cam recording continues in the background. A recording indicator is enough; do not cover the projection image with a camera preview.

### F. Dash cam

The Camera Module 3 is a forward-facing dash cam, not a cabin camera and not a CarPlay camera.

- Record while the unit is on, including while Android Auto or CarPlay is on the display.
- Segment the recording (on the order of one to three minutes) and delete the oldest segments when the footage disk reaches its limit.
- Default capture around 1080p30 or 1296p30. Full 4608×2592 stills are available but are not the loop-recording mode. The standard lens is about a 75° diagonal field of view, so the housing aim matters: a wide module was not detected.
- Use autofocus for the road ahead. Lock or bias focus toward distance once framing is known, so it does not hunt on the dashboard reflection.
- Write clips to a dedicated USB volume, not to the 29 GB OS card.
- Flush segments so a power cut loses at most the open clip.
- Optional on-screen time stamp in the recording.
- A review page on the small display, for use while parked, can wait until the live UI and the loop itself work.

Out of scope until asked: cloud upload, parking-mode sentry (that needs always-on power and an ignition signal), a second camera, and an IMU trigger. This board has no accelerometer.

### G. Power and heat

- Prefer a supply that can signal shutdown before voltage disappears.
- Keep the active cooler's intake and exhaust open in the printed housing.
- Watch throttling once display, phone video, and camera encode run together. `vcgencmd get_throttled` was clean at idle; that will not stay meaningful in a closed car.

## 6. Application architecture

Proposed layout. Change it if the Android Auto or CarPlay engine wants a different boundary.

```text
raspi-headunit/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── docs/
│   ├── architecture.md
│   ├── hardware.md
│   ├── setup.md
│   └── roadmap.md
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── ApplicationManager.cpp
│   │   └── ConfigurationManager.cpp
│   ├── connection/
│   │   ├── PhoneConnectionManager.cpp
│   │   ├── USBManager.cpp
│   │   └── WiFiManager.cpp
│   ├── bluetooth/
│   │   └── BluetoothManager.cpp
│   ├── audio/
│   │   └── AudioRouter.cpp
│   ├── projection/
│   │   ├── ProjectionManager.cpp
│   │   ├── AndroidAutoReceiver.cpp
│   │   └── CarPlayReceiver.cpp
│   ├── video/
│   │   └── VideoRenderer.cpp
│   ├── camera/
│   │   ├── DashcamRecorder.cpp
│   │   └── StorageManager.cpp
│   └── input/
│       └── TouchInputManager.cpp
├── ui/
│   ├── Main.qml
│   ├── HomePage.qml
│   ├── ConnectionPage.qml
│   ├── DashcamPage.qml
│   └── SettingsPage.qml
├── config/
├── scripts/
├── tests/
└── systemd/
```

Keep the UI, the projection engines, and the dash cam behind separate interfaces. Recording must keep running when the screen is given to the phone.

## 7. Development roadmap

### Phase 0 — Header display

Done for the panel. The hat is the ILI9486 DRM device at 480×320 on labwc output `SPI-2`, with ADS7846 touch and the backlight on. `config.txt` has `dtoverlay=piscreen,drm,rotate=180,invx,invy`. The glass is rotated 180 from the driver default, and touch X/Y are swapped and both inverted so a tap matches that view.

`~/.config/labwc/rc.xml` maps `ADS7846 Touchscreen` to `SPI-2` with mouse emulation. labwc was started with `--merge-config`, and `labwc --reconfigure` (with `LABWC_PID` set) reloaded that file without replacing `/etc/xdg/labwc/rc.xml`. `lcd-touch.service` is disabled and the debug canvas is not running. Leave `rotate`, `invx`, and `invy` alone unless a tap is mirrored. A tap through the ADS7846 opened the panel’s terminal and was then closed; the desktop was left on the wallpaper.

**Deliverable:** The hat is the desktop display after power-on, and a tap clicks it.

### Phase 1 — Fullscreen shell

- Qt 6 QML is installed from Bookworm. The fullscreen shell is in `raspi-headunit/` at 480×320, with connection, dash-cam status, and settings pages.
- `raspi-headunit/systemd/headunit.service` can start that shell with the graphical session. It is not enabled. The labwc desktop stays up until someone asks for the shell at boot.

**Deliverable:** The shell comes up on the attached display after power-on. Not yet: the unit is deliberately not enabled.

### Phase 2 — Bluetooth audio to the car

- Turn Bluetooth on. Pair to a speaker or the car stereo as an A2DP source.
- Play a local file through PipeWire to that sink.
- Measure reconnect and latency.

**Deliverable:** The Pi reliably plays audio on the car stereo.

### Phase 3 — Dash cam

- Record from the IMX708 with libcamera and hardware encode.
- Segment files onto a USB disk, with oldest-out deletion.
- Run a recording while the Qt shell is fullscreen.
- Confirm a hard power loss only drops the open segment.
- Set focus and exposure for a windshield view. Confirm the image is the road, not the mirror housing, once the board is held in the intended orientation.

**Deliverable:** Unattended loop recording from the rear-facing camera, alongside the UI.

### Phase 4 — Wired Android Auto

- Pick a receiver stack after checking license, Pi 5 support, and build status.
- USB session, video on the small display, touch back to the phone, audio into the Bluetooth path.
- Dash cam keeps recording during the session.

**Deliverable:** Wired Android Auto, touch, car audio, and dash cam together.

### Phase 5 — Wireless Android Auto

- Discovery, Wi-Fi transport, automatic reconnect.
- Same video, touch, audio, and dash-cam behavior as wired.

**Deliverable:** Wireless Android Auto that comes back after the phone drops.

### Phase 6 — Apple CarPlay

- Only with a real MFi path.
- Wired, then wireless, using the same display, touch, and audio routes.

**Deliverable:** CarPlay where the authentication hardware exists.

### Phase 7 — Car fit and housing

- Boot time, touch latency, Bluetooth latency, thermal soak.
- Ignition-style shutdown.
- Printed housing: driver display, camera out the windshield toward the mirror side, cooler vents, mirror mount.
- Fetch or draw the print file from the measured stack. This phase is a conversation of its own.

**Deliverable:** A unit that can live at the mirror, record the road, and show the phone UI.

## 8. Constraints

- Free and open-source software. No HUDIY or other paid app as a core dependency.
- Do not modify the car stereo.
- The Pi is the hub for the phone, the display, the dash cam, and Bluetooth audio.
- Wired and wireless phone links both stay in the design. Wireless is the goal, USB is the fallback.
- Keep modules separate. Mark features as working, experimental, or planned.
- Document licenses and any MFi requirement.
- Reliability, a short boot, a readable daylight screen, and an uncorrupted last dash-cam clip matter more than extra features.
- The camera variant is known. The display is the ILI9486 hat at 480×320. The printable shell is still later.

## 9. Instructions for the coding agent

Act as a senior embedded Linux and C++ engineer. Implement this incrementally on this Pi.

**First task, after this plan:**

1. Phase 0 is already up: `ili9486` at 480×320 on `/dev/fb0`, ADS7846 touch, and `dtoverlay=piscreen,drm,rotate=180,invx,invy` in `config.txt`. After the next reboot, only change rotation or `invx`/`invy` if the picture or a tap is still sideways. Do not switch it to a DSI or HDMI display.
2. Record the detected camera, port, cooler, disk, and display in `docs/hardware.md`.
3. Scaffold the C++20 / Qt 6 tree in section 6, including the dash-cam module boundaries, without implementing projection protocols or the recorder yet.
4. Add CMake, a fullscreen QML shell, config, and logging.
5. Add Pi 5 setup notes and a short build/run section.
6. Add a few unit tests where they are honest (config, storage-limit math). Do not pretend to unit-test the sensor.

Do not implement Android Auto, CarPlay, or the printed housing in that first pass. Do not start a dash-cam encoder until the shell builds, unless a tiny libcamera smoke test is needed to prove the sensor is still there.

When a protocol detail or a Pi feature is uncertain, check it on this board or in current sources. The hardware table in section 2 is the baseline; re-probe if a cable moves.
