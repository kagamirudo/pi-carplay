# Architecture

The shell, the projection engines, and the dash cam are separate objects owned by `ApplicationManager`. A phone session can take the screen without destroying `DashcamRecorder`.

Android Auto runs in a separate process, the patched OpenAuto `autoapp`. The shell does not link it. It only signals it (`ShellPresenter::openAndroidAuto` sends `SIGUSR1`). See the [system architecture](../../docs/architecture.md). Nothing in this tree opens the IMX708 yet.

| Module | Role in this build |
| --- | --- |
| `ConfigurationManager` | JSON config load, save, and defaults. Tested. |
| `Logger` | One JSON object per line. |
| `PhoneConnectionManager` | Link state. "Look for phone" reports that no receiver exists. |
| `USBManager` | Lists USB product names from sysfs. No phone protocol. |
| `WiFiManager` | Reads `wlan0` operstate. No Wi-Fi Direct. |
| `BluetoothManager` | Reads rfkill for the status line. |
| `BluetoothDevices` | QML-facing device list. Scans and connects through `bluetoothctl`, pairs over D-Bus, and routes PipeWire output to the device. |
| `BluetoothAgent` | `org.bluez.Agent1`. Answers PIN, passkey and confirmation requests only for the device being paired. |
| `AudioRouter` | Names the route. PipeWire is not connected. |
| `ProjectionManager` | Holds the two receivers. Screen owner stays `shell`. |
| `AndroidAutoReceiver`, `CarPlayReceiver` | Stubs. `start()` returns false. |
| `VideoRenderer` | Reports that no frame is available. |
| `DashcamRecorder` | Types and status only. `start()` does not encode or write a file. |
| `StorageManager` | Oldest-out math and the "not the OS disk" check. Tested. No deletes. |
| `TouchInputManager` | Device name, `SPI-1`, event-node lookup. Does not grab the device. While labwc owns the seat, the shell gets pointer events. |
| QML | Home, Phone, Cam, BT, and Setup at 480×320. |

Dash-cam policy, once a footage disk exists: segments on the order of one to three minutes, reserve bytes kept free, oldest segments dropped until the next segment fits. A limit of 0 or a path on the OS volume is refused. The encoder that would call this is not built.
