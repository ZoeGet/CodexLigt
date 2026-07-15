# CodexLight

[简体中文](README.md) | English

CodexLight is a Codex status light project based on ESP32-C3 SuperMini. It uses three independent WS2812B LEDs to show the current Codex state, and a desktop-side Bridge monitors local Codex Desktop logs.

Two connection methods are supported:

- Wired USB serial: the desktop bridge automatically finds the ESP32 serial port and sends `GREEN` / `RED` / `YELLOW`
- Wireless UDP: the desktop bridge broadcasts state packets on the LAN, and the ESP32 listens over Wi-Fi

Both methods can be enabled at the same time, or you can choose only one.

## State Mapping

| Light | Codex state |
| --- | --- |
| Green | idle, completed, or no recent agent activity |
| Red | thinking, writing, running tools, or taking action |
| Yellow | waiting for approval or explicit user input |

## Current Features

- PlatformIO firmware project under `Firmware/`.
- Arduino Framework and FastLED are integrated.
- `LedController` wraps LED control so business logic does not call FastLED directly.
- Three independent single-pixel WS2812B LEDs are supported.
- USB serial state commands are supported.
- Wi-Fi UDP state commands are supported.
- Desktop Bridge monitors local Codex JSONL/SQLite logs.
- Desktop Bridge supports automatic serial detection, UDP broadcast, and Win10 tray background mode.
- Firmware has been verified with `pio run`.
- Hardware schematic files are under `Hardware/Schematic/`.

## Hardware Configuration

MCU: ESP32-C3 SuperMini

LEDs: three independent WS2812B LEDs. They are not connected as a chained strip. Each LED has its own data line, and each channel uses `LEDS_PER_CHANNEL = 1`.

| LED | GPIO | Color |
| --- | --- | --- |
| Red | GPIO7 | Red |
| Green | GPIO6 | Green |
| Yellow | GPIO5 | Yellow |

Hardware connection notes:

- Each WS2812B DIN line has a 330 ohm series resistor.
- Each WS2812B has a 100nF decoupling capacitor near its power pins.
- LED power ground and ESP32-C3 ground must be connected together.

## Project Structure

```text
CodexLight/
├── README.md
├── README.en.md
├── .gitignore
├── Bridge/
│   ├── CodexLightTray.ps1
│   ├── README.md
│   ├── README_en.md
│   ├── codex_light_monitor.py
│   └── start_codex_light_tray.bat
├── Docs/
├── Firmware/
│   ├── platformio.ini
│   ├── include/
│   │   ├── config.h
│   │   ├── led.h
│   │   └── wifi_secrets.example.h
│   └── src/
│       ├── main.cpp
│       └── led.cpp
└── Hardware/
    └── Schematic/
        └── Schematic1.pdf
```

Directory overview:

- `Bridge/`: desktop-side bridge program that monitors Codex logs and sends light states.
- `Docs/`: project documentation directory.
- `Firmware/`: ESP32-C3 firmware project.
- `Hardware/`: schematic and future hardware files.

## Desktop Bridge

The bridge script is located at:

```text
Bridge/codex_light_monitor.py
```

It monitors local Codex logs:

```text
C:\Users\<you>\.codex\sessions\YYYY\MM\DD\rollout-*.jsonl
C:\Users\<you>\.codex\logs_2.sqlite
```

### Console Mode

Monitor and print states only:

```powershell
python Bridge\codex_light_monitor.py
```

Use USB serial:

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200
```

Use UDP broadcast:

```powershell
python Bridge\codex_light_monitor.py --udp --udp-port 4210
```

Enable both USB serial and UDP:

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200 --udp --udp-port 4210
```

### Win10 Tray Background Mode

Double-click:

```text
Bridge\start_codex_light_tray.bat
```

By default, it enables both wired serial and wireless UDP:

```bat
set "MONITOR_ARGS=--serial auto --baud 115200 --udp --udp-port 4210"
```

The tray icon appears in the folded notification area at the right side of the Win10 taskbar. Right-click it to open the log folder, restart the monitor script, or exit the background program.

### Bridge Protocols

USB serial output:

```text
GREEN
RED
YELLOW
```

UDP output is sent to `255.255.255.255:4210` by default, and the current state is repeated every 2 seconds as a heartbeat. After pairing, UDP packets include a token:

```text
CODEXLIGHT/1 token=<paired-token> GREEN
CODEXLIGHT/1 token=<paired-token> RED
CODEXLIGHT/1 token=<paired-token> YELLOW
```

For first-time wireless UDP use, put the ESP32 into pairing mode, then run:

```powershell
python Bridge\codex_light_monitor.py --pair --udp-port 4210
```

After pairing succeeds, the token is stored in ESP32 NVS, while the desktop stores the token, ESP32 MAC, and recent IP in `Bridge\config.local.json`; it does not need to be hard-coded. During normal operation, the Bridge prefers unicast state packets to the saved ESP32 IP and only accepts matching-MAC `HELLO` packets to refresh that IP.

Serial mode requires pyserial:

```powershell
pip install pyserial
```

UDP mode has no third-party Python dependency.

## Firmware

The firmware project is located in `Firmware/`:

```ini
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
lib_deps = fastled/FastLED
```

`Firmware/include/config.h` centralizes hardware and communication parameters:

- `RED_LED_PIN = 7`
- `GREEN_LED_PIN = 6`
- `YELLOW_LED_PIN = 5`
- `LEDS_PER_CHANNEL = 1`
- `DEFAULT_BRIGHTNESS = 64`
- `SERIAL_BAUD = 115200`
- `UDP_PORT = 4210`
- `WIRELESS_TIMEOUT_MS = 10000`

`Firmware/include/led.h` defines the public `LedController` API, including single-state display methods:

```cpp
void showRed();
void showGreen();
void showYellow();
```

`Firmware/src/main.cpp` handles both input paths:

- `GREEN` / `RED` / `YELLOW` from `Serial`
- `CODEXLIGHT/1 GREEN` / `RED` / `YELLOW` from UDP

After receiving a valid state, the firmware lights only the matching LED.

## Wi-Fi Configuration

Wireless UDP mode requires Wi-Fi credentials. Copy:

```text
Firmware\include\wifi_secrets.example.h
```

to:

```text
Firmware\include\wifi_secrets.h
```

Then fill in:

```cpp
#define CODEXLIGHT_WIFI_SSID "YourWiFiName"
#define CODEXLIGHT_WIFI_PASSWORD "YourWiFiPassword"
```

`wifi_secrets.h` is ignored by Git and will not be committed to GitHub. Without this file, the firmware still builds and works over USB serial. The UDP token is stored in ESP32 NVS through pairing and does not need to be written to `wifi_secrets.h`.

## Verification

Desktop script syntax check:

```powershell
python -m py_compile Bridge\codex_light_monitor.py
```

Firmware build:

```powershell
cd Firmware
pio run
```

## Hardware Files

Hardware files are located under `Hardware/`:

- `Hardware/Schematic/Schematic1.pdf`: current schematic file.

## License

MIT
