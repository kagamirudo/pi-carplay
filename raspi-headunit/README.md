# raspi-headunit

The head-unit shell: a full-screen Qt 6 / QML app for the Raspberry Pi 5 and its 3.5″ ILI9486 panel (480×320, labwc output `SPI-1`). It is the home screen that Android Auto (`autoapp`) projects on top of.

| Page | Contents |
| --- | --- |
| Home | **Android Auto** button (brings projection back), phone, dash-cam and display status |
| Phone | Projection state, USB / Wi-Fi / Bluetooth / audio summaries |
| Cam | Dash-cam status and storage policy |
| BT | Bluetooth scan, pair (PIN agent), connect, disconnect, forget; routes audio to the device |
| Setup | Log level, segment length, storage limit, save |

## Build

From the repository root, `make headunit test` does everything below. By hand:

```sh
cmake -S raspi-headunit -B raspi-headunit/build -DCMAKE_BUILD_TYPE=Release
cmake --build raspi-headunit/build -j4
ctest --test-dir raspi-headunit/build --output-on-failure
QT_QPA_PLATFORM=offscreen raspi-headunit/build/headunit --smoke
```

Requires Qt 6.4 (Core, DBus, Gui, Qml, Quick) and the QML modules listed in the root `Makefile` (`make deps`).

## Run

At login it is started by `~/.config/labwc/autostart` (installed by `make install`). By hand on the running session:

```sh
raspi-headunit/scripts/run-shell.sh
```

## Configuration

Read from `~/.config/raspi-headunit/headunit.json` if present, otherwise `config/headunit.json`. **Save** on the Setup page writes the file under `~/.config`. Logs are JSON lines on stderr. Set `logging.path` to also append to a file.

## Implementation status

| Area | State |
| --- | --- |
| Config, storage policy, QML shell | Implemented and unit-tested |
| Bluetooth list, pairing agent, audio routing | Implemented (`src/bluetooth/`) |
| Android Auto | Provided by the patched OpenAuto `autoapp` (see `../patches/`). The shell starts and resumes it. |
| USB / Wi-Fi status lines | sysfs and rfkill snapshots |
| `AndroidAutoReceiver`, `CarPlayReceiver`, `DashcamRecorder` | Interfaces only: no in-process protocol or encoder |

Dash-cam clips must go on a USB disk, not the OS card. `StorageManager` refuses a path on the OS volume.

## systemd

`systemd/headunit.service` is an optional user unit, an alternative to the labwc autostart. It is not installed or enabled by default. See `docs/setup.md`.

More docs: [hardware](docs/hardware.md) · [architecture](docs/architecture.md) · [roadmap](docs/roadmap.md) · [setup](docs/setup.md)
