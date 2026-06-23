# Celestial-Wayfinder

**Empower off-grid communication.** The Celestial Wayfinder is a handheld, battery-powered beacon that
shares your GPS location and short status messages over a **LoRa mesh** — no cell network required — and
points you toward other users (or saved waypoints) with an **LED compass ring**.

![The Celestial Wayfinder](https://hackster.imgix.net/uploads/attachments/1671493/_zM7FUV5gcx.blob?auto=compress%2Cformat&w=900&h=675&fit=min)

This repository is the **application / firmware layer**. Nearly all of the reusable machinery — display
stack, LED engine, LoRa mesh, navigation, settings, RPC — lives in the companion
[**esp32-utilities**](https://github.com/Blake-Ballew/esp32-utilities) library. This project supplies the
*concrete hardware drivers*, the *bootstrap wiring*, and the *Wayfinder-specific UI and messages* that
turn that library into a finished device.

---

## Hardware

The device is built around a custom 4-layer mainboard integrating the ESP32, peripheral connectors,
interface components, and power management.

### Power architecture

Powered by a single **18650 lithium-ion** cell. A charge IC (TP4056 on early revisions; a **BQ25672**
PMIC on v3) handles charging, and a **solid-state soft power switch** lets the device shut itself down —
like a laptop power button — including an automatic cutoff when the cell drops too low (over-discharging
li-ion permanently damages the cell). On battery, a DC-DC converter generates the main 5 V rail; the LEDs
run directly from 5 V.

### Display & LEDs

| | |
|---|---|
| ![128xN OLED home screen](https://hackster.imgix.net/uploads/attachments/1669921/oled_kk2vDHTSdD.jpg?auto=compress%2Cformat&w=740&h=555&fit=max) | A monochrome OLED shows the home screen and menus (128×32 on v1, 128×64 on v2, 128×128 SH1107 on v3). |
| ![LED compass ring](https://hackster.imgix.net/uploads/attachments/1669927/led_compass_0YRWSKrjx6.jpg?auto=compress%2Cformat&w=740&h=555&fit=max) | A ring of WS2812B LEDs is the compass display, pointing toward a target bearing. |
| ![LED flashlight](https://hackster.imgix.net/uploads/attachments/1669945/led_flashlight_Hl6dHK2f0o.jpg?auto=compress%2Cformat&w=740&h=555&fit=max) | The LED array doubles as a flashlight. |

### Radio

![LoRa radio](https://hackster.imgix.net/uploads/attachments/1669970/antenna_LBvCVSCt8A.jpg?auto=compress%2Cformat&w=740&h=555&fit=max)

A LoRa radio (SX127x-class, 915 MHz) sends and receives packets carrying GPS coordinates and status
messages between users, forming an ad-hoc mesh.

### Input

Four face buttons and a rotary encoder (with detent handling). Inputs are captured by ISRs and pushed
onto the display command queue.

---

## Hardware revisions

The firmware targets three board revisions, selected at compile time via the `HARDWARE_VERSION` macro and
a matching PlatformIO environment:

| Revision | `HARDWARE_VERSION` | MCU / Board | OLED | Notable hardware |
|---|---|---|---|---|
| v1 | `1` | ESP32 (`esp32dev`) | 128×32 SSD1306 | HMC5883 compass |
| v2 | `2` | ESP32 (`esp32dev`) | 128×64 SSD1306 | 31-LED ring |
| v3 | `3` | ESP32-**S3** (`esp32-s3-devkitm-1`) | 128×128 SH1107 | BQ25672 PMIC, 61 LEDs, I²C peripheral auto-detect, IMU |

On v3, peripherals are discovered by scanning the I²C bus at boot, so the compass/IMU driver and even the
OLED-vs-virtual-display choice adapt to whichever chips are populated.

---

## Build

Built with **PlatformIO** using the **ESP-IDF** framework (with the Arduino component layered on top).

```bash
# Build & flash a v3 device
pio run -e hardware-v3 -t upload

# Build for the older ESP32 boards
pio run -e local-library-v1
pio run -e local-library-v2

# Serial monitor (115200 baud)
pio device monitor
```

Environments are defined in [`platformio.ini`](platformio.ini): `local-library-v1`, `local-library-v2`,
`hardware-v3`, and `hardware-v3-tag-connect`. A pre-build script
([`generate_version.py`](generate_version.py)) stamps the firmware version from
[`version_config.json`](version_config.json) (currently **3.7.8**) into a generated header.

> **The esp32-utilities dependency is consumed as a local symlink** (`symlink://../esp32-utilities` in
> `lib_deps`), so the two repositories are expected to sit side by side. Changes to the library are picked
> up directly — no publish step.

### Key dependencies

| Dependency | Role |
|---|---|
| [esp32-utilities](https://github.com/Blake-Ballew/esp32-utilities) | All core modules (display, LED, LoRa, navigation, settings, RPC) |
| FastLED | WS2812B LED control |
| TinyGPSPlus | GPS NMEA parsing |
| Adafruit GFX + SSD1306 / SH110X | OLED drivers |
| Adafruit HMC5883 / MMC56x3 / LIS2MDL / BMM350 | Magnetometer options across revisions |
| ArduinoJson 7.x | Settings + MessagePack serialization |
| ESP32Encoder | Rotary encoder input |
| BQ25672 | v3 battery/power-management IC |
| [arduino-LoRa (fork)](https://github.com/Blake-Ballew/arduino-LoRa.git) | SX127x LoRa radio driver |

---

## Software architecture

The application follows the same layered, namespaced, header-only conventions as esp32-utilities. Its job
is to **inject concrete hardware behind the library's interfaces** and **assemble the UI**.

```
main.cpp
   └─ Bootstrap()                       ← selects the V1/V2/V3 bootstrap set by HARDWARE_VERSION
        ├─ BootstrapMicrocontroller     ← pins, buses (SPI/I²C), encoder, PMIC, battery, power mgmt
        ├─ BootstrapLeds                ← FastLED, LED segments, patterns, input/shutdown animations
        ├─ BootstrapNavigation          ← compass driver, GPS, time & location sources
        ├─ BootstrapDisplay             ← OLED driver, DisplayModule::Manager, input-label layout, UI
        ├─ BootstrapLora                ← LoRa driver, radio tasks, PingMessage type, mesh wiring
        └─ BootstrapRpc                 ← RPC manager + transports, registered RPC functions
```

After bootstrap, everything is **task- and event-driven** — the Arduino `loop()` does nothing. Inputs flow
from ISRs into the display command queue; the display manager task renders; LoRa runs on its own core; RPC
polls its channels.

### Bootstrap pattern

Each subsystem has a per-revision bootstrap class under [`include/Bootstrap/`](include/Bootstrap) (`V1`,
`V2`, `V3`, plus `Common`). They are static classes whose hardware instances are Meyers singletons
(`static T& Foo()`), so a board's entire pinout and peripheral wiring lives in one header.
[`main.cpp`](src/main.cpp) `#include`s the correct set and calls each `Initialize()` in dependency order.

### Hardware abstraction (the library "plugs")

The app implements the library's interfaces with real drivers:

| Library interface | Wayfinder implementation |
|---|---|
| `CompassInterface` | `CompassV1` / `CompassV2` / `CompassV3` (auto-selects the populated magnetometer) |
| `LoraDriverInterface` | `ArduinoLoRaDriver` (SX127x via the arduino-LoRa fork; CAD-based clear-channel sensing) |
| `GeolocationInterface` | `GpsSource` (TinyGPS++ on `Serial2`) with a `StaticLocation` fallback |
| `TimeSourceInterface` | `GpsSource` (GPS time) and `EzTimeSource` |

### LoRa messaging — `PingMessage`

The app defines one LoRa message type, [`PingMessage`](include/HelperClasses/PingMessage.hpp): sender
name, theme color (RGB), latitude/longitude, and a short status string. Its schema GUID is the FNV-1a hash
of its sorted payload keys (`"abgnors"`), and it's registered with the library's message factory so
incoming packets deserialize polymorphically.
[`WayfinderLoraState`](include/HelperClasses/WayfinderLoraState.hpp) tracks **unread messages**
(thread-safe, mutex-guarded) and the user's **saved status-message list** (persisted to SPIFFS), and
exposes RPC handlers for both.

### Display & UI

The UI is a stack of windows built on `DisplayModule`. The
[`HomeWindow`](include/HelperClasses/Window/HomeWindow.hpp) is the heart of the app — it owns a web of
states implementing the core flows:

- **Broadcast** — pick a location (current GPS or a saved waypoint) → pick a status message → transmit a
  `PingMessage` to the mesh.
- **Read incoming pings**, reply, and **save** a sender's message or location.
- **Lock screen** (enables power savings), resend last broadcast, quick-action menu.

The **main menu** (wired in [`CompassUtils`](include/CompassUtils.h)) reaches Settings, WiFi/Bluetooth
remote configuration, status-message and saved-location editors, compass/GPS/diagnostics debug screens, a
**Breakout** mini-game on v3, reboot, and shutdown. App-specific windows and draw commands (e.g.
`CompassWindow`, `BreakoutWindow`, `PairBluetoothWindow`, `LineCompassDrawCommand`, the `Flashlight` LED
pattern) live under [`include/HelperClasses/`](include/HelperClasses).

### Settings, RPC & remote configuration

`CompassUtils` generates the device's typed settings (user name, device name, theme color, WiFi mode, …),
persists them to NVS, and fans `SettingsUpdated` out to the System, LoRa, Bluetooth, and Connectivity
modules. The same settings and device state are reachable **remotely over RPC** — an async web server (port
80, CORS-enabled), USB serial, and BLE all share one function registry (saved locations, saved messages,
settings, OTA firmware update, and system info/restart), enabling a companion configuration app.

### Power management

The soft power switch and PMIC integrate with the library's lifecycle events: `systemShutdown` drives the
PMIC into ship mode (after a shutdown LED fade played *first* via `PushFront`), and the lock screen toggles
power savings — dropping the CPU to 80 MHz and cutting LED power — restoring full speed on unlock.

---

## Project structure

```
Celestial-Wayfinder/
├── include/
│   ├── Bootstrap/              # Per-revision hardware wiring (Common, V1, V2, V3)
│   ├── HelperClasses/
│   │   ├── Compass/            # CompassV1/V2/V3 (CompassInterface implementations)
│   │   ├── LoRaDriver/         # ArduinoLoRaDriver (LoraDriverInterface)
│   │   ├── Led/Patterns/       # Flashlight + app LED patterns
│   │   ├── DrawCommands/       # LineCompassDrawCommand, …
│   │   ├── Window/  + States/  # HomeWindow, CompassWindow, BreakoutWindow, … and their states
│   │   ├── PingMessage.hpp     # The Wayfinder LoRa message type
│   │   └── WayfinderLoraState.hpp  # Unread + saved status-message state
│   ├── CompassUtils.h          # App-level glue: settings, menus, RPC registration
│   ├── EventDeclarations.h     # Input ISRs, shared task/queue handles
│   └── globalDefines.h         # Per-revision pin maps & build constants
├── src/                        # main.cpp, CompassUtils.cpp, EventDeclarations.cpp
├── platformio.ini              # Build environments (v1, v2, v3, v3-tag-connect)
├── version_config.json         # Firmware version source (→ generate_version.py)
└── partition_table.csv         # Custom flash partition layout
```

---

## License

See [LICENSE](LICENSE).
</content>
