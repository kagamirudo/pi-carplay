# Usage

## Screens

The shell has five tabs across the top:

| Tab | What it shows |
| --- | --- |
| **Home** | **Android Auto** button, phone link, dash-cam and display status |
| **Phone** | Android Auto / CarPlay state, USB, Wi-Fi, Bluetooth and audio summaries |
| **Cam** | Dash-cam status and storage limits |
| **BT** | Bluetooth device list: scan, connect, disconnect, forget |
| **Setup** | Log level, segment length, storage limit, save settings |

## Android Auto

- **Start:** plug in the phone. Projection starts on its own and covers the shell.
- **Leave:** open the Android Auto app grid (⋮⋮, bottom right) and tap **Exit**. The projection hides, and the session and audio keep running.
- **Come back:** shell **Home** → **Android Auto**. The phone resumes the picture.

The phone draws a 720×480 layout that the Pi scales down to the 480×320 panel. Text and icons stay at phone-like proportions.

## Bluetooth (car stereo or speaker)

1. Open **BT** and tap **Scan**. The scan runs for 25 seconds and the list refreshes as devices appear.
2. Put the car or speaker in pairing mode.
3. Tap **Connect** next to it.

During pairing the shell answers the device on its own:

- **PIN request** (older car stereos): it sends `0000`, then retries with `1234` if that is refused.
- **Numeric comparison** (newer cars): it accepts. Confirm the 6-digit code on the car if asked.
- Only the device you tapped is answered. Pairing requests from anything else are rejected.

After pairing, the device is marked *trusted* so it reconnects on its own. The Pi's default audio output moves to it, along with anything already playing. **Disconnect** and **Forget** are on each row.

## Command line

```sh
make restart    # restart the shell and Android Auto, re-plug the phone
make status     # running processes, supervisors, power, display overlay, recent crashes
make selftest   # full on-device health check (also: pi-carplay-selftest)
```

Logs are in `~/.local/state/pi-carplay/`: `autoapp.log` (Android Auto), `headunit.log` (shell) and `crashes.log` (every restart). Logs rotate at 2 MiB.

### Optional colour correction

The projection can be gamma, contrast and saturation corrected for your panel. It is off by default. Set the variables in `~/.config/labwc/autostart` before `./autoapp`:

```sh
OPENAUTO_PANEL_GAMMA=1.1 OPENAUTO_PANEL_CONTRAST=1.1 OPENAUTO_PANEL_SATURATION=1.2 \
OPENAUTO_NO_MAIN_WINDOW=1 ./autoapp
```

`1.0` means no change. Gamma above 1 darkens mid-tones.
