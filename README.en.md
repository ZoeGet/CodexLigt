# CodexLight

English | [简体中文](README.md) | [English Usage Guide](USAGE.en.md) | [中文使用说明](USAGE.md)

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
- BOM, PCB, schematic, Gerber, and 3D-printable enclosure files.

## State Mapping

| State | Codex condition | LED | GPIO |
| --- | --- | --- | --- |
| `GREEN` | Idle, task completed, or task aborted | Green | GPIO6 |
| `RED` | Reasoning, responding, running tools, or processing a task | Red | GPIO7 |
| `YELLOW` | Waiting for approval, permission, or explicit user input | Yellow | GPIO5 |
| Disconnected | No valid desktop heartbeat for 6 seconds | Yellow, one-second blink cycle | GPIO5 |

When the first valid heartbeat arrives, GPIO6 blinks for two seconds as a connection indication. After the animation, only the LED for the actual state remains on. After a task completes, the green frame remains latched until another task starts, the Bridge enters a waiting state, or the connection is lost.

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

The current global brightness is `25/255`, configured by `DEFAULT_BRIGHTNESS` in [Firmware/include/config.h](Firmware/include/config.h).

Hardware files:

- `Hardware/BOM/BOM.xlsx`: bill of materials
- `Hardware/Schematic/Schematic1.pdf`: schematic
- `Hardware/PCB/Source/CodexLight.epro2`: PCB source project
- `Hardware/PCB/Gerber/CodexLight_PCB_Gerber.zip`: Gerber package
- `Hardware/Enclosure/`: top and bottom STL files

## Repository Layout

```text
CodexLight/
├─ Bridge/                 # Desktop log monitor, serial/UDP sender, Windows tray app
├─ Firmware/               # ESP32-C3 PlatformIO firmware
├─ Hardware/
│  ├─ BOM/                 # Bill of materials
│  ├─ Schematic/           # Circuit schematic
│  ├─ PCB/                 # PCB source and Gerber fabrication files
│  └─ Enclosure/           # 3D-printable top and bottom enclosure
├─ Docs/                   # Usage and implementation documentation
├─ README.md               # Chinese documentation
├─ README.en.md            # English documentation
├─ USAGE.md                # Chinese usage guide
├─ USAGE.en.md             # English usage guide
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

## Usage Guides

The complete operating workflow is available in the root usage guides:

- [English Usage Guide](USAGE.en.md)
- [中文使用说明](USAGE.md)

They cover firmware build and upload, phone AP provisioning, wired/wireless/AUTO modes, Windows tray switching, serial and UDP protocols, state rules, troubleshooting, security, and development verification.

Quick tray startup:

```text
Bridge\start_codex_light_tray.bat
```

## Documentation

- [Bridge Chinese Guide](Bridge/README.md)
- [Bridge English Guide](Bridge/README_en.md)
- [English Usage Guide](USAGE.en.md)
- [中文使用说明](USAGE.md)
- [Usage and Implementation Guide](Docs/USAGE_AND_IMPLEMENTATION.md)
- [使用与实现说明](Docs/使用与实现说明.md)

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
