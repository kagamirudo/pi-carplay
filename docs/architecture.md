# Architecture

Two programs share one screen under the labwc Wayland compositor.

| Process | Language / license | Role |
| --- | --- | --- |
| `headunit` | C++20, Qt 6.4 QML, MIT | The always-running shell: navigation, Bluetooth, status, settings, and later the dash cam |
| `autoapp` | C++14, Qt 5, GPL-3.0 (OpenAuto + aasdk, patched) | Android Auto receiver: USB/AOAP, TLS, H.264 video, audio, touch |

They are separate processes on purpose. A crash or restart of the projection never takes down the shell or, later, the dash-cam recorder. The GPL code also stays in its own binary.

## Startup

`~/.config/labwc/autostart` starts `headunit` first, then `autoapp` with `OPENAUTO_NO_MAIN_WINDOW=1`, so OpenAuto's own launcher never shows. When a phone connects, `autoapp` maps a frameless, always-on-top, full-screen window over the shell.

## Video path

```
phone ── H.264 800×480 (80 px side margins) ──► aasdk ──► QtVideoOutput
   QMediaPlayer → QGraphicsVideoItem → QGraphicsView
   sceneRect = 720×480 content area, transform = uniform ×2/3
   └─► 480×320 window on labwc output SPI-1 ──► ili9486 DRM ──► SPI ──► panel
```

The phone is told the screen is 800×480 with an 80 px width margin, so it lays out a **720×480** UI (3:2, the panel's aspect ratio). The Pi crops the margins and scales evenly by 2/3 with smooth filtering. A 1:1 crop of the centre makes icons too large and overlapping. A non-uniform stretch distorts them.

## Touch path

```
ADS7846 (spi0.1) ─► libinput (calibrationMatrix from rc.xml) ─► labwc ─► focused window
   └─ autoapp InputDevice: window point → 720×480 content coordinates → phone
```

The calibration matrix fixes the hat's flipped axis. OpenAuto reports a 720×480 touchscreen to the phone, matching the content area.

## Switching between shell and Android Auto

| Direction | Mechanism |
| --- | --- |
| Android Auto → shell | User taps **Exit** in Android Auto. The phone sends `VideoFocusRequest(UNFOCUSED)`. `VideoService` answers `UNFOCUSED` and hides the projection window, and the shell underneath is visible. |
| Shell → Android Auto | **Android Auto** button runs `pkill -USR1 -x autoapp`. The signal handler sets a flag. A 200 ms Qt timer shows the window and sends an unrequested `FOCUSED` indication, and the phone resumes the stream. |

## Bluetooth

- `BluetoothDevices` (shell) lists devices from `bluetoothctl`: connected first, then paired, then nearby named devices.
- `BluetoothAgent` exports `org.bluez.Agent1` on the system bus at `/headunit/bluetooth/agent` and registers as the default agent with `KeyboardDisplay` capability.
- Pairing calls `org.bluez.Device1.Pair` asynchronously with a 60 s timeout. On an authentication failure it retries with the next PIN (`0000`, then `1234`).
- After pairing, a small script trusts and connects the device. It then waits for the `bluez_output.*` PipeWire sink, makes it the default and moves active streams to it.

## Shell internals

See [`raspi-headunit/docs/architecture.md`](../raspi-headunit/docs/architecture.md) for the module breakdown: `ApplicationManager`, configuration, storage policy, connection managers and the QML pages.
