# Hardware

Reference unit, checked October 2026.

| Item | Detail |
| --- | --- |
| Board | Raspberry Pi 5 Model B Rev 1.0, 4 GB, Cortex-A76 |
| OS | Debian 12 Bookworm (Raspberry Pi OS), kernel `6.12.x+rpt-rpi-2712` |
| OS disk | ~32 GB microSD. Dash-cam footage does not belong on it. |
| Display | 3.5″ SPI hat, ILI9486, **480×320**, 73×49 mm, `ili9486` DRM driver via `spi0.0`. Wayland output `SPI-1`. The name can change between kernels, and was `SPI-2` on an earlier boot. |
| Overlay | `dtoverlay=piscreen,drm,speed=32000000,rotate=180,invx,invy` |
| SPI clock | Requested 32 MHz, which is the binding maximum. RP1 divides 200 MHz by 8, so **25 MHz actual**. |
| Touch | `ADS7846 Touchscreen` on `spi0.1`, IRQ GPIO17. libinput `calibrationMatrix 1 0 0 0 -1 1` in `~/.config/labwc/rc.xml`, with mouse emulation on. |
| Camera | Raspberry Pi Camera Module 3 (Sony IMX708, autofocus VCM `dw9817`) on **CAM/DISP 1**. Still modes 1536×864, 2304×1296, 4608×2592. |
| Bluetooth | Onboard, BlueZ 5.66, audio via PipeWire (`bluez_output.*` sinks) |
| Cooling | Official active cooler (`pwm-fan`) |
| Power | **5 V / 5 A required.** Undervoltage whites out the panel and can reboot the Pi. |

## Header pinout used by the overlay

| Signal | GPIO (BCM) |
| --- | --- |
| LCD chip select | GPIO8 (CE0) |
| Touch chip select | GPIO7 (CE1) |
| MOSI / MISO / SCLK | GPIO10 / GPIO9 / GPIO11 |
| LCD data/command | GPIO24 |
| LCD reset | GPIO25 |
| Backlight | GPIO22 |
| Touch IRQ | GPIO17 |

This is the Waveshare 3.5″ RPi LCD / OzzMaker PiScreen pinout. The board puts an SPI-to-16-bit converter in front of the ILI9486, so each command byte is sent as a 16-bit SPI word. The kernel driver handles that.

## Sources

- Driver: [`drivers/gpu/drm/tiny/ili9486.c`](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/drivers/gpu/drm/tiny/ili9486.c)
- Overlay: [`piscreen-overlay.dts`](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/arch/arm/boot/dts/overlays/piscreen-overlay.dts)
- Binding (max SPI frequency): [`ilitek,ili9486.yaml`](https://www.kernel.org/doc/Documentation/devicetree/bindings/display/ilitek%2Cili9486.yaml)
