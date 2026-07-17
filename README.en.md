# CodexLight

English | [简体中文](README.md)

CodexLight is an ESP32-C3 status light for Codex Desktop. A desktop Bridge reads local Codex session logs and sends the current state to an ESP32 over USB serial or UDP on the same LAN. The ESP32 drives three independent WS2812B LEDs for working, waiting, and completed states.

This is an independent community project and is not officially affiliated with or endorsed by OpenAI.

## Features

- USB serial and Wi-Fi UDP transports.
- Persistent `AUTO`, `WIRED`, and `WIRELESS` transport modes.
- Phone-based Wi-Fi provisioning through an ESP32 access point and web portal.
- Non-blocking provisioning, so USB serial remains available without Wi-Fi.
- A state heartbeat every 2 seconds and a 6-second firmware link timeout.
- A two-second green connection animation before the actual Codex state is shown.
- Optional Windows system tray operation.
- PCB, schematic, Gerber, and 3D-printable enclosure files.

## State Mapping

| State | Codex condition | LED | GPIO |
| --- | --- | --- | --- |
| `GREEN` | Idle, task completed, or task aborted | Green | GPIO6 |
| `RED` | Reasoning, responding, running tools, or processing a task | Red | GPIO7 |
| `YELLOW` | Waiting for approval, permission, or explicit user input | Yellow | GPIO5 |
| Disconnected | No valid desktop heartbeat for 6 seconds | Yellow, one-second blink cycle | GPIO5 |

When the first valid heartbeat arrives, GPIO6 blinks for two seconds as a connection indication. After the animation, only the LED for the actual state remains on.

## Hardware

### Recommended Parts

- ESP32-C3 SuperMini or a compatible ESP32-C3 development board
- Three WS2812B RGB LEDs
- A stable 5 V power supply
- A USB data cable

### Wiring

| Function | ESP32-C3 pin |
| --- | --- |
| Yellow LED DIN | GPIO5 |
| Green LED DIN | GPIO6 |
| Red LED DIN | GPIO7 |
| WS2812B VCC | 5 V |
| WS2812B GND | GND |

The LEDs use three independent data inputs; they are not a chained strip. The firmware matches the reference hardware with `NEO_GRB + NEO_KHZ800`, while color values are passed in standard RGB order. If another LED batch displays incorrect colors, change the pixel order in [Firmware/src/led.cpp](Firmware/src/led.cpp).

Hardware files:

- `Hardware/Schematic/Schematic1.pdf`: schematic
- `Hardware/PCB/Source/CodexLight.epro2`: PCB source project
- `Hardware/PCB/Gerber/CodexLight_PCB_Gerber.zip`: Gerber package
- `Hardware/Enclosure/`: top and bottom STL files

## Repository Layout

```text
CodexLight/
├─ Bridge/                 # Desktop log monitor, serial/UDP sender, Windows tray app
├─ Firmware/               # ESP32-C3 PlatformIO firmware
├─ Hardware/               # Schematic, PCB, and enclosure assets
├─ Docs/                   # Usage and implementation documentation
├─ README.md               # Chinese documentation
├─ README.en.md            # English documentation
└─ LICENSE                 # MIT License
```

## Requirements

### Desktop

- Windows 10 or Windows 11
- Python 3.9 or newer
- Codex Desktop
- `pyserial` for wired operation

```powershell
python -m pip install pyserial
```

### Firmware

- PlatformIO Core or the VS Code PlatformIO IDE
- Espressif32 Platform
- Arduino Framework

PlatformIO installs these dependencies automatically:

- `Adafruit NeoPixel`
- `WiFiManager`

## Build and Upload

Run from PowerShell:

```powershell
cd C:\path\to\CodexLight\Firmware
pio run
pio run -t upload --upload-port COM4
```

Replace `COM4` with the actual ESP32-C3 port. The firmware uses USB CDC at `115200` baud.

Open the serial monitor:

```powershell
pio device monitor --port COM4
```

Expected startup output:

```text
CODEXLIGHT READY
STATUS mode=WIRED active=NONE wifi=DISCONNECTED
```

Exit the monitor with `Ctrl+C`. PlatformIO Monitor and the Bridge cannot own the same COM port at the same time.

## Wi-Fi Provisioning

### First-Time Setup

When no usable Wi-Fi credentials are available, the ESP32 starts this configuration network:

```text
SSID: CodexLight-XXXX
Password: 123456789
URL: http://192.168.4.1
```

1. Connect a phone to `CodexLight-XXXX`.
2. If the captive portal does not open automatically, browse to `http://192.168.4.1`.
3. Select a 2.4 GHz Wi-Fi network available to both the ESP32 and the computer.
4. Enter the Wi-Fi password and save.
5. The configuration AP closes after a successful connection. Future boots reconnect automatically.

The ESP32-C3 supports 2.4 GHz Wi-Fi only. Wireless control requires the computer and ESP32 to be on the same LAN without client isolation.

### Change the Configuration AP

Edit [Firmware/include/config.h](Firmware/include/config.h):

```cpp
constexpr const char* CONFIG_AP_SSID_PREFIX = "CodexLight";
constexpr const char* CONFIG_AP_PASSWORD = "123456789";
```

The AP password must contain at least eight characters. Rebuild and upload the firmware after changing it.

### Clear or Reconfigure Wi-Fi

Send these commands through the serial monitor:

```text
WIFI_CONFIG
CLEAR_WIFI
```

- `WIFI_CONFIG` opens the provisioning AP.
- `CLEAR_WIFI` removes saved Wi-Fi credentials and opens the provisioning AP.

## Transport Modes

| Mode | Behavior |
| --- | --- |
| `WIRED` | Accept USB serial only; this is the current default |
| `WIRELESS` | Accept UDP on the same LAN only |
| `AUTO` | Accept serial and UDP; a valid serial heartbeat has priority |

Runtime commands:

```text
MODE WIRED
MODE WIRELESS
MODE AUTO
STATUS
```

The selected mode is stored in ESP32 NVS and normally survives firmware uploads. The Bridge sends `MODE WIRED` when only serial is enabled and `MODE AUTO` when serial and UDP are enabled together, so normal operation does not require erasing NVS.

The firmware default is configured in [Firmware/include/config.h](Firmware/include/config.h):

```cpp
constexpr const char* DEFAULT_TRANSPORT_MODE = "WIRED";
```

## Run the Bridge

Run these commands from the repository root.

### Wired USB

```powershell
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200
```

Automatic serial selection is also available:

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200
```

Specify the COM port explicitly when multiple serial devices are connected.

### Wireless UDP

```powershell
python Bridge\codex_light_monitor.py --udp --udp-port 4210
```

The Bridge initially uses UDP broadcast for discovery. After receiving an ESP32 `HELLO`, it stores the device MAC and recent IP in the Git-ignored `Bridge/config.local.json`, then prefers unicast.

### Enable Both Transports

```powershell
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200 --udp --udp-port 4210
```

When serial and UDP are enabled together, the Bridge automatically sends `MODE AUTO`. The firmware then selects a transport from fresh heartbeats and gives wired traffic priority when both are valid.

### Windows Tray Mode

Double-click:

```text
Bridge\start_codex_light_tray.bat
```

The default configuration enables automatic serial and UDP. The tray menu can open logs, restart the Bridge, or exit. Edit `MONITOR_ARGS` in the batch file to change its arguments.

## Serial Commands

| Command | Effect |
| --- | --- |
| `GREEN` | Set the wired state to green and refresh its heartbeat |
| `RED` | Set the wired state to red and refresh its heartbeat |
| `YELLOW` | Set the wired state to yellow and refresh its heartbeat |
| `PING` | Refresh the wired heartbeat and reply with `PONG` |
| `STATUS` | Print mode, active transport, Wi-Fi state, and IP |
| `MODE WIRED` | Use serial only and persist the mode in NVS |
| `MODE WIRELESS` | Use UDP only and persist the mode in NVS |
| `MODE AUTO` | Accept serial and UDP and persist the mode in NVS |
| `WIFI_CONFIG` | Open the Wi-Fi provisioning AP |
| `CLEAR_WIFI` | Clear Wi-Fi credentials and open the AP |

## Protocol

Serial uses one ASCII command per line:

```text
GREEN\n
RED\n
YELLOW\n
```

UDP state packets:

```text
CODEXLIGHT/1 GREEN
CODEXLIGHT/1 RED
CODEXLIGHT/1 YELLOW
```

The ESP32 broadcasts discovery packets every two seconds:

```text
CODEXLIGHT/1 HELLO mac=AA:BB:CC:DD:EE:FF mode=WIRELESS
```

The default UDP port is `4210`, the desktop heartbeat interval is 2 seconds, and the firmware link timeout is 6 seconds.

## State Detection

The Bridge monitors Codex JSONL session logs under `~/.codex/sessions`:

- `task_started`, reasoning, messages, tool calls, and tool outputs: `RED`
- Tool calls requiring approval, permission, or user input: `YELLOW`
- `task_complete` or `turn_aborted`: `GREEN`

An active task does not turn green because of an ordinary `item/completed` event or a brief period without log output.

## Troubleshooting

### The Bridge connects but the yellow LED keeps blinking

1. Confirm that the latest firmware is uploaded.
2. Close PlatformIO Monitor before starting the Bridge.
3. Use an explicit port such as `--serial COM4`.
4. Send `STATUS` and confirm the mode is `WIRED` or `AUTO`.
5. Allow the two-second green connection animation to finish.

### A phone cannot join the provisioning AP

- Confirm the password is `123456789`, or check the value in `config.h`.
- Disable automatic Wi-Fi switching or mobile-data assistance temporarily.
- Open `http://192.168.4.1` manually.
- Send `CLEAR_WIFI` over serial and retry.

### Wi-Fi is configured but wireless control does not work

- Put the computer and ESP32 on the same 2.4 GHz LAN.
- Allow Python to use UDP port 4210 through the firewall.
- Disable AP/client isolation on the router.
- Send `MODE WIRELESS` or `MODE AUTO`.
- Delete `Bridge/config.local.json` to force device rediscovery.

### LED colors are incorrect

The current hardware uses `NEO_GRB`. For another LED batch, try `NEO_RGB` or the appropriate order in `Firmware/src/led.cpp`, then rebuild and upload.

### PlatformIO reports multiple Core installations

This warning comes from multiple PlatformIO Core installations on the computer, not from the firmware. Follow the troubleshooting link printed by PlatformIO and remove the obsolete installation.

## Development Verification

```powershell
# Bridge syntax check without writing __pycache__
python -B -c "import ast,pathlib; ast.parse(pathlib.Path('Bridge/codex_light_monitor.py').read_text(encoding='utf-8')); print('OK')"

# Firmware build
cd Firmware
pio run -j 1
```

## Documentation

- [Bridge Chinese Guide](Bridge/README.md)
- [Bridge English Guide](Bridge/README_en.md)
- [Refactored Usage Guide](Docs/重构后使用说明.md)
- [Current Implementation Summary](Docs/当前实现说明.md)
- [Implementation Status](Docs/IMPLEMENTATION_STATUS.md)

## Security

The current UDP control protocol is not encrypted or authenticated. Use it only on a trusted LAN, change the default provisioning AP password, and do not expose the device to public or untrusted networks.

## Contributing

Issues and pull requests are welcome. Before submitting a change:

- Verify the Bridge with a Python syntax check.
- Build the firmware with `pio run`.
- Update both Chinese and English documentation for behavior changes.
- Do not commit Wi-Fi passwords, device IP addresses, or other local configuration.

## License

This project is licensed under the [MIT License](LICENSE). Third-party dependencies remain subject to their own licenses.
