# Wireless Android Auto

**Status: preparation.** The hardware checks, the USB Bluetooth adapter choice and the 5 GHz hotspot profile are ready. The Bluetooth handshake that tells the phone to join the hotspot is not built yet, so the phone still connects over USB. This page explains what wireless needs, what to buy, how to set it up, and what is left.

## What wireless Android Auto needs

| Piece | What happens | State here |
| --- | --- | --- |
| Bluetooth pairing | The phone pairs with the head unit like a hands-free kit. | ✅ The shell's pairing agent handles it |
| Bluetooth trigger | The head unit connects the phone's hands-free (HFP/HSP) profile. The phone's Android Auto then opens an RFCOMM channel to the head unit's service `4de17a00-52cb-11e6-bdf4-0800200c9a66`. | ❌ Not built |
| Wi-Fi handshake | Over RFCOMM, framed as `[length u16][message id u16][protobuf]` (big-endian): head unit sends `WifiStartRequest` (1) with its IP and TCP port, phone asks `WifiInfoRequest` (2), head unit answers `WifiInfoResponse` (3) with SSID, WPA2 key and BSSID, phone reports `WifiConnectStatus` (6) and `WifiStartResponse` (7). | ❌ Not built |
| Wi-Fi link | The phone joins an access point on the head unit: **5 GHz**, WPA2-PSK, fixed head-unit IP. | ✅ Profile prepared by `pi-carplay-wireless-setup` (not started) |
| Projection | The phone opens TCP to the IP and port from step 3 and runs the same Android Auto protocol as over USB (TLS, video, audio, touch). | ✅ `autoapp` already listens on TCP 5000 |
| Calls | Call audio goes over Bluetooth HFP, with the Pi as the hands-free side. | ❌ Not built |

The port is whatever the head unit announces. OpenAuto's `autoapp` uses **5000**. [WirelessAndroidAutoDongle](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle) and [aa-proxy-rs](https://github.com/aa-proxy/aa-proxy-rs) use 5288.

The phone needs Android 11 or later with Android Auto, and 5 GHz Wi-Fi.

### What the OpenAuto build here already has

The pinned openDsh OpenAuto (`e7caeb4`) has two wireless pieces:

- **TCP transport: works.** `App::waitForWirelessDevice()` accepts a session on TCP port 5000 and runs it over aasdk's `TCPTransport`. You can see it on a running Pi: `ss -ltnp` shows `autoapp` on `0.0.0.0:5000`.
- **Bluetooth handshake: present but unused.** `btservice/` (`AndroidBluetoothService`, `AndroidBluetoothServer`) is compiled into `libopenauto`, but `autoapp` never creates it. openDsh's `dash` front end uses it, and this project does not. It also cannot be switched on as it is:
  - it announces the last IPv4 address of *any* interface (on this Pi that would be Ethernet or Tailscale, not the hotspot)
  - it hard-codes port 5000
  - it reads messages without using the length field
  - it uses the first Bluetooth controller, whichever that is
  - it writes the Wi-Fi password to the log
  - it never makes the HFP connection that prompts the phone

`autoapp` also listens on port 5000 on **every** interface today, so any device on the same network could start a session. The roadmap below binds it to the hotspot address.

## Choosing a USB Bluetooth adapter

TP-Link only supports Windows officially. On Linux, support comes from the kernel's `btusb` driver plus firmware. Product names get reused across hardware revisions, so trust the USB ID that `lsusb` shows, not the box.

| Model | USB ID | Chipset | Firmware | On this Pi (Bookworm, kernel 6.12) |
| --- | --- | --- | --- | --- |
| **UB500** | `2357:0604` | Realtek RTL8761BU, BT 5.0 | `rtl_bt/rtl8761bu_fw.bin` and `rtl8761bu_config.bin` from `firmware-realtek` | ✅ **Recommended.** `btusb` knows the ID and the firmware is installed |
| **UB500 Plus** | `2357:0604` | Same, with an external antenna | Same | ✅ **Best for the car.** Same driver, more range |
| UB5A | `2357:0604` (as reported) | Same | Same | ✅ Same hardware as the UB500 |
| UB400 / UB4A | `0a12:0001` | CSR8510 A10, BT 4.0 | None | ✅ Works with the generic driver. Older, and cheap clones share the ID |
| UB600 | `37ad:0600` | Realtek RTL8761BUV | Same Realtek files | ❌ **Don't buy.** This kernel's `btusb` lacks the ID, which arrived in Linux 7.2, so the firmware never loads |

`pi-carplay-wireless-check` reads the installed `btusb` module and prints this verdict for the current kernel, so you can check before buying.

Known issues:

- Early Pi 5 bootloaders made Realtek dongles fail to load firmware about half the time (`RTL: download fw command failed (-110)`, [raspberrypi/linux#6141](https://github.com/raspberrypi/linux/issues/6141)). The RP1 fix shipped in the late-2024 bootloaders. This Pi runs 2025-05-08. The check warns about older bootloaders.
- Some users report RTL8761BU adapters reconnecting now and then on certain firmware versions. The check prints the loaded firmware version (`RTL: fw version …`) so it can be compared.
- Plug the dongle into a USB 2.0 (black) port, or use a short extension away from the USB 3.0 ports. USB 3.0 radiates noise at 2.4 GHz.

### Do you need a second adapter?

The Pi 5's onboard CYW43455 (Bluetooth 5.0 on the UART, `hci0`) can do wireless Android Auto by itself. A USB adapter helps when:

- **Splitting the roles.** Onboard stays with the car stereo (A2DP, as today), and the dongle serves the phone (HFP plus the Android Auto RFCOMM channel). Scanning, pairing and reconnecting the stereo then never disturb the phone link.
- **Coexistence.** The onboard Bluetooth shares one chip and antenna with the Wi-Fi that will carry the projection video. A separate Bluetooth radio removes that airtime sharing.
- **Range.** Behind a mirror in a metal car body, the UB500 Plus antenna helps.

### Picking which controller does what

BlueZ has no "preferred controller" setting. The first controller registered becomes the default (`hci0`), and the shell's Bluetooth page uses `hci0`.

- **Keep both (recommended).** Onboard registers first at boot, so it stays `hci0` for the car stereo. `pi-carplay-wireless-setup` records the dongle's address in `~/.config/pi-carplay/wireless.conf`, and the wireless service will bind to that address. `pi-carplay-wireless-check` warns if the dongle ever takes `hci0`. By hand: `bluetoothctl list`, then `bluetoothctl select <address>`.
- **Dongle only.** Add `dtoverlay=disable-bt` to `/boot/firmware/config.txt`, run `sudo systemctl disable hciuart`, and reboot. The dongle becomes `hci0` and the shell uses it for everything. The project's tools never edit `config.txt` for this. Do it yourself only if you want it.

## Setting up

When the dongle arrives:

```sh
lsusb | grep -iE '2357:0604|0a12:0001'         # the adapter is seen
journalctl -k -b | grep -i 'RTL: fw version'    # Realtek firmware loaded (UB500)
pi-carplay-wireless-check                       # or: make wireless-check
pi-carplay-wireless-setup --dry-run             # preview (or: make wireless-setup ARGS=--dry-run)
pi-carplay-wireless-setup --channel 149         # record the dongle and write the hotspot profile
pi-carplay-wireless-check                       # the dongle and profile now PASS
```

`pi-carplay-wireless-setup`:

1. Installs `firmware-realtek` if missing (`--no-firmware` skips it).
2. Records the first USB Bluetooth controller as the phone adapter (or `--adapter AA:BB:CC:DD:EE:FF`) in `~/.config/pi-carplay/wireless.conf`.
3. Writes the NetworkManager profile `pi-carplay-aa` (mode 0600, in `/etc/NetworkManager/system-connections/`): access point, 5 GHz, WPA2-PSK with a random 20-character key, `10.42.0.1/24` with DHCP for the phone, **autoconnect off**.

It never starts the hotspot and never touches the current Wi-Fi connection. Running it again keeps the password and the profile's UUID. `--remove` deletes both files.

### Channel and country

The check lists the 5 GHz channels that need no radar detection on this Pi (36–48 and 149–165 here) and fails if the regulatory country is unset. Set the country with `raspi-config`: Localisation Options, WLAN Country. In Vietnam (`VN`) the regulatory database marks 36–48 as indoor-only, so use **`--channel 149`**.

### Trying the hotspot by hand

The Pi 5's Wi-Fi can run an access point and a client only on the same channel. **Starting the hotspot drops any Wi-Fi client connection on `wlan0`.** If you reach the Pi over Wi-Fi, use Ethernet or the console first.

```sh
nmcli connection up pi-carplay-aa       # start
nmcli connection down pi-carplay-aa     # stop
sudo nmcli -s -g 802-11-wireless-security.psk connection show pi-carplay-aa   # the password
```

A phone can join it, but Android Auto won't start until the Bluetooth handshake exists.

## Roadmap

1. ✅ **Hardware readiness.** Adapter choice, firmware, the check tool, the hotspot profile.
2. **Bluetooth handshake service.** A separate process, like the shell and `autoapp` are, that:
   - binds to the adapter in `wireless.conf`
   - registers the Android Auto RFCOMM profile and an HSP/HFP profile with BlueZ (`org.bluez.ProfileManager1`)
   - connects to the paired phone
   - runs the five-message handshake with the hotspot's address, SSID, key and BSSID

   It will be written from the protocol and tested over a private D-Bus like `tst_bluetooth_agent`. WirelessAndroidAutoDongle's `bluetoothHandler.cpp` and `bluetoothProfiles.cpp` show the sequence.
3. **Hotspot lifecycle.** Start `pi-carplay-aa` at boot when wireless is enabled. Stop it when a phone is plugged in over USB.
4. **`autoapp` patch** (GPL-3.0, in `patches/`): listen only on the hotspot address, take the port from config, and handle reconnects after the phone drops.
5. **Calls.** HFP hands-free role on the Pi through PipeWire's native backend, on the phone adapter.
6. **Shell.** Wireless status and an on/off switch on the Phone page, and a `wireless` section in `headunit.json`.
7. **Hardware checklist.** Rows for wireless connect, reconnect after the phone drops, and a call.

USB stays as the fallback throughout.

## References

- [nisargjhaveri/WirelessAndroidAutoDongle](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle): turns a Pi into a wireless-to-USB Android Auto dongle. It is the clearest reference for the Bluetooth sequence and messages.
- [aa-proxy-rs](https://github.com/aa-proxy/aa-proxy-rs): a Rust take on the same idea.
- [openDsh/openauto](https://github.com/openDsh/openauto) `btservice/`: the unused handshake described above.
- [raspberrypi/linux#6141](https://github.com/raspberrypi/linux/issues/6141): the Pi 5 Realtek firmware-load issue and its bootloader fix.
