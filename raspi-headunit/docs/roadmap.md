# Roadmap

| Phase | State |
| --- | --- |
| 0 Display and touch | **Done.** ILI9486 on labwc output `SPI-1` at 480×320, 25 MHz SPI. ADS7846 calibrated through libinput. |
| 1 Shell | **Done.** Qt 6 QML shell: Home, Phone, Cam, BT, Setup. Starts at login under the Android Auto projection. |
| 2 Bluetooth audio | **Working.** Scan, pair with an automatic PIN agent (`0000`/`1234`), trust, connect, route PipeWire output. Tested with a speaker. Car stereo still to be tested. |
| 3 Dash cam | **Started.** Camera Module 3 verified with `rpicam-still`. Storage policy written and tested. Loop recorder (libcamera H.264 segments to USB disk) not built. |
| 4 Wired Android Auto | **Working.** Patched OpenAuto + aasdk: 720×480 layout scaled to the panel, touch, Exit to shell and back. |
| 5 Wireless Android Auto | Planned. Needs Wi-Fi AP plus Bluetooth RFCOMM handshake. |
| 6 CarPlay | Planned, using an MFi-certified adapter only. No MFi bypass. |
| 7 Housing and car fit | Planned. Mirror-mount enclosure with the camera on the back face. |

## Next up

- Automatic recovery when the Android Auto session drops (TLS read error 26 or USB transfer error 10) instead of `make restart`.
- Dash-cam loop recorder, kept running independently of projection.
- Auto-reconnect of the car stereo at start-up.
