# Troubleshooting

## White screen, random reboots

**Cause:** undervoltage. A power dip resets the ILI9486 controller to a blank white state. The driver doesn't re-initialise it on its own, and bigger dips reboot the Pi. A phone charging from the Pi's USB port makes it worse.

Check:

```sh
vcgencmd get_throttled     # anything other than 0x0 means undervoltage happened
dmesg | grep -i voltage
```

**Fix:** use a 5 V / 5 A supply (the official 27 W one, or a quality 5 A car converter). To recover the panel without restarting anything:

```sh
wlr-randr --output SPI-1 --off; sleep 1; wlr-randr --output SPI-1 --on
```

Switching the output off and on re-runs the panel's init sequence.

## Black screen while Android Auto is "running"

The video stream stalled. This usually follows a dropped session (`SSL_READ` error 26 or USB transfer error 10 in `~/.local/state/pi-carplay/autoapp.log`). Fix:

```sh
make restart
```

It restarts both programs and resets the phone's USB port through sysfs `authorized`.

## Phone connects but Android Auto never starts

- `lsusb` should show `18d1:2d00` (accessory mode) after a few seconds. If the phone stays in MTP mode (for example `04e8:6860` on Samsung), run `make restart`.
- Check `/etc/udev/rules.d/51-android-accessory.rules` is installed (`make install-udev`). Without it `autoapp` cannot open the device.
- The desktop's MTP helper (`gvfsd-mtp`) can grab the phone first. A USB reset through `make restart` usually wins it back.
- Accept the Android Auto prompt on the phone the first time.

## Touch is offset or mirrored

Adjust `calibrationMatrix` in `deploy/labwc/rc.xml` (see [installation](installation.md#7-first-run)), then:

```sh
make install-labwc
kill -HUP "$(pgrep -x labwc)"
```

If labwc ignores `SIGHUP` because it inherited an ignored signal, log out and back in instead.

## Colours look washed out or bluish

The panel is a low-cost TN LCD, so colour and contrast shift strongly with viewing angle. Look at it straight on and tilt the mount towards the driver.

Leave the optional software correction off unless it clearly helps ([usage](usage.md#optional-colour-correction)). Darkening mid-tones on this panel pushes orange towards red and greys towards blue.

## SPI speed

The ILI9486 devicetree binding allows **at most 32 MHz**. The Pi 5's SPI controller divides a 200 MHz clock by an even number, so the actual rate is:

| `speed=` | Divider | Actual | Notes |
| --- | --- | --- | --- |
| 48000000 | 6 | 33.3 MHz | Over spec. Smoother, but risks corrupted pixels |
| **32000000** | 8 | **25 MHz** | Default: fastest within spec |
| 24000000 | 10 | 20 MHz | Noticeably choppier |

Change it in `/boot/firmware/config.txt` and reboot.

## Bluetooth device won't pair

- Make sure the device is in pairing mode **before** tapping Connect, and that it showed up in the last scan. Otherwise pairing reports "out of range".
- The status line shows which PIN was tried. If the car wants a different fixed PIN, edit `pins_` in `raspi-headunit/src/bluetooth/BluetoothDevices.hpp`.
- If the shell shows `Pairing agent: …` at start-up, another program holds the default agent. The shell's agent takes over by default, so restart the shell after closing the other program.

## USB Bluetooth adapter not working

Run `pi-carplay-wireless-check`. It names each controller, says whether `btusb` knows the adapter, and shows the firmware result from the kernel log.

- `firmware file … not found`: install `firmware-realtek` (`pi-carplay-wireless-setup` does this) and replug.
- `download fw command failed (-110)` on a Pi 5: update the bootloader (`sudo rpi-eeprom-update -a`, then reboot) and replug.
- No firmware line at all for a Realtek adapter: this kernel may not know its USB ID. The TP-Link UB600 (`37ad:0600`) is one example. Use a UB500 (`2357:0604`).
- The shell's BT page lists devices on `hci0`. If the dongle took `hci0`, see [picking which controller does what](wireless.md#picking-which-controller-does-what).

## The shell or Android Auto keeps restarting

Both programs run under `pi-carplay-supervise`, which starts them again when they exit: after 1 s, then doubling up to 30 s while they keep failing. Each exit is a line in `~/.local/state/pi-carplay/crashes.log`. Run `pi-carplay-selftest` and look at the end of the matching log. Under-voltage (`vcgencmd get_throttled` not `0x0`) is the most common cause.

If the Setup page says **Settings were reset**, the saved `headunit.json` could not be read. It was moved to `headunit.json.corrupt` next to it and the shell started on defaults.

## Logs

| File | Contents |
| --- | --- |
| `~/.local/state/pi-carplay/autoapp.log` | OpenAuto / aasdk session log |
| `~/.local/state/pi-carplay/headunit.log` | Shell (JSON lines) |
| `~/.local/state/pi-carplay/crashes.log` | Every exit or crash of either program, with time and signal; the supervisor restarted each one |
| `journalctl -u bluetooth` | BlueZ |
