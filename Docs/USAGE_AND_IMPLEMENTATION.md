# CodexLight Usage and Implementation Guide

English | [简体中文](使用与实现说明.md) | [Documentation Index](README.md) | [Project Home](../README.en.md)

This document combines the operating guide and technical implementation reference for firmware flashing, phone-based AP provisioning, wired or wireless operation, troubleshooting, and code maintenance. The root [README.en.md](../README.en.md) remains the main project entry point.

## Hardware Connections

| LED | Data pin | State |
| --- | --- | --- |
| Yellow WS2812B | GPIO5 | Disconnected or waiting for approval |
| Green WS2812B | GPIO6 | Connection animation or task completed |
| Red WS2812B | GPIO7 | Codex is working |

Each WS2812B uses an independent DIN. Connect VCC to a stable 5 V supply, share ground with the ESP32-C3, and keep the schematic's 330 ohm series resistor on each DIN line.

## Build and Upload

```powershell
cd C:\path\to\CodexLight\Firmware
pio run -j 1
pio run -t upload --upload-port COM4
```

Replace `COM4` with the actual port. Monitor startup output with:

```powershell
pio device monitor --port COM4 --baud 115200
```

Normal startup output is similar to:

```text
CODEXLIGHT READY
STATUS mode=WIRED active=NONE wifi=DISCONNECTED
```

Exit the monitor with `Ctrl+C`. PlatformIO Monitor and the Bridge cannot use the same serial port simultaneously.

## Wired Connection

Run from the repository root:

```powershell
python -m pip install pyserial
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200
```

Automatic ESP32 serial selection is also available:

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200
```

The Bridge repeats `MODE WIRED` until the ESP32 replies with `MODE_OK WIRED`, then sends the current state heartbeat every two seconds.

## Phone AP Provisioning

When no usable Wi-Fi credentials exist, the ESP32 opens:

```text
SSID: CodexLight-XXXX
Password: 123456789
URL: http://192.168.4.1
```

1. Connect the phone to `CodexLight-XXXX`.
2. Wait for the captive portal, or open `http://192.168.4.1` manually.
3. Select a 2.4 GHz Wi-Fi network also reachable by the computer.
4. Enter its password and save.
5. After connecting, the ESP32 closes the temporary AP and reconnects automatically on later boots.

The ESP32-C3 does not support 5 GHz Wi-Fi. To provision again, send over serial:

```text
WIFI_CONFIG
CLEAR_WIFI
```

`WIFI_CONFIG` forces the portal open. `CLEAR_WIFI` removes saved credentials and then opens the portal.

## Wireless Connection

Make sure the computer and ESP32 are on the same LAN, then run:

```powershell
python Bridge\codex_light_monitor.py --udp --udp-port 4210
```

Before the first wireless-only session, save this mode over serial:

```text
MODE WIRELESS
```

The Bridge initially discovers the ESP32 by UDP broadcast. It stores the matching MAC address and recent IP in the Git-ignored `Bridge/config.local.json`, then prefers unicast.

## Wired and Wireless Together

```powershell
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200 --udp --udp-port 4210
```

The Bridge automatically sends `MODE AUTO`. `AUTO` accepts both heartbeat sources, prefers a fresh wired connection, and can continue wirelessly after the wired heartbeat expires.

## Windows Tray Startup

Double-clicking `Bridge\start_codex_light_tray.bat` starts in `AUTO` mode. A mode can also be selected from PowerShell:

```powershell
Bridge\start_codex_light_tray.bat auto
Bridge\start_codex_light_tray.bat wired
Bridge\start_codex_light_tray.bat wireless
```

`wired` enables serial only. In `wireless`, state traffic uses UDP only; when USB is available, the Bridge first saves `MODE WIRELESS` over serial and releases the port after acknowledgement, while without USB it uses the firmware's saved mode. The batch file centralizes serial port, baud rate, and UDP port settings at the top. After startup, the tray's `Connection mode` submenu switches directly between `Auto`, `Wired only`, and `Wireless only`, automatically restarting the Bridge while the tray status text shows the selected mode.

## Transport Modes and NVS

```text
MODE WIRED
MODE WIRELESS
MODE AUTO
STATUS
```

The mode is stored under Preferences namespace `codexlight`, key `transport`. A normal firmware upload does not erase NVS. For a complete erase:

```powershell
cd Firmware
pio run -t erase --upload-port COM4
pio run -t upload --upload-port COM4
```

## LED Behavior

- No valid computer heartbeat: GPIO5 yellow blinks with a one-second full period.
- First computer connection: GPIO6 green blinks for two seconds.
- Codex reasoning, responding, or running tools: GPIO7 red stays on.
- Waiting for approval, permission, or user input: GPIO5 yellow stays on.
- `task_complete` or `turn_aborted`: GPIO6 green stays on.
- No heartbeat on the active transport for six seconds: returns to blinking yellow.

The Bridge latches approval-related `YELLOW` against ordinary parallel tool events and SQLite diagnostics until the matching approval call returns. Completion-related `GREEN` similarly ignores trailing events from the completed turn until the next `task_started` or `user_message`.

The firmware caches the last LED frame and resends it once per second to correct occasional WS2812B latch corruption caused by power or timing interference.

## Architecture

```text
Codex Desktop logs
        |
        v
Bridge/codex_light_monitor.py
        |
        +-- USB CDC serial: GREEN / RED / YELLOW
        +-- LAN UDP: CODEXLIGHT/1 <STATE>
                         |
                         v
                  ESP32-C3 firmware
                         |
                         +-- GPIO5 yellow WS2812B
                         +-- GPIO6 green WS2812B
                         +-- GPIO7 red WS2812B
```

## Bridge Implementation

The Bridge reads:

- `~/.codex/sessions/**/*.jsonl`
- `~/.codex/logs_2.sqlite`

It uses event types, tool names, and diagnostic levels to determine the state. It sends only color states to the ESP32, never Codex message text, tool output, API keys, or login tokens.

| Codex event | Output |
| --- | --- |
| `task_started`, messages, reasoning, tool calls, and tool outputs | `RED` |
| A call requires approval, permission, or user input | `YELLOW` |
| `task_complete` or `turn_aborted` | `GREEN` |

After opening serial, the Bridge waits two seconds before mode negotiation and state output. Serial and UDP repeat the current state every two seconds by default.

## Firmware Implementation

The entry point is `Firmware/src/main.cpp`. Main modules are:

- `src/config_portal.cpp`: non-blocking WiFiManager AP provisioning.
- `src/led.cpp`: three independent Adafruit NeoPixel outputs.
- `include/config.h`: GPIO, brightness, port, timeout, AP, and default-mode settings.

Each loop iteration handles serial, WiFiManager, UDP, active transport selection, and LED rendering. There is no infinite Wi-Fi wait and no `while (!Serial)`, so wired operation remains available without a network.

The LED driver uses `NEO_GRB + NEO_KHZ800`, matching the current hardware. Every rendered state updates all three channels and explicitly clears inactive LEDs.

## Protocol

Serial commands:

```text
GREEN
RED
YELLOW
PING
STATUS
MODE WIRED
MODE WIRELESS
MODE AUTO
WIFI_CONFIG
CLEAR_WIFI
```

UDP states and discovery:

```text
CODEXLIGHT/1 GREEN
CODEXLIGHT/1 RED
CODEXLIGHT/1 YELLOW
CODEXLIGHT/1 PING
CODEXLIGHT/1 HELLO mac=<MAC> mode=<MODE>
```

## Troubleshooting

### Yellow keeps blinking

- The Bridge is not running, the monitor owns the serial port, or the COM port is wrong.
- Firmware is saved as `WIRELESS`, but only the wired Bridge is running.
- In wireless mode, the computer and ESP32 are not on the same LAN.

### Yellow does not stay on for approval

- Restart the Bridge to ensure it is running the latest `codex_light_monitor.py`.
- PowerShell should print `YELLOW approval_needed:<tool>`.
- In tray mode, right-click the icon and choose `Restart monitor`.

### The phone cannot connect to the provisioning AP

- Confirm the AP password matches `Firmware/include/config.h` and has at least eight characters.
- Temporarily disable automatic network switching or mobile-data assistance.
- Open `http://192.168.4.1` manually.
- Send `CLEAR_WIFI` over serial and retry.

### Colors are incorrect

The current hardware uses `NEO_GRB`. For a different LED batch, verify the WS2812B color order, DIN orientation, GPIO continuity, and soldering.

## Security and Local Configuration

- `Bridge/config.local.json`, `Bridge/logs/`, and `Firmware/include/wifi_secrets.h` are Git-ignored.
- WiFiManager debug output is disabled, and firmware does not print Wi-Fi passwords over serial.
- UDP control is neither encrypted nor authenticated; use it only on a trusted LAN.
- The default provisioning password is public configuration. Change it in `Firmware/include/config.h` before deploying a real device.

## Removed Legacy Behavior

- Runtime Wi-Fi credentials hard-coded through `wifi_secrets.h`.
- A custom blocking configuration page.
- UDP token pairing and `PAIR_SET`.
- FastLED global multi-controller refresh.
- Treating ordinary `item/completed` events as full-turn completion.

## Verification and Maintenance

```powershell
python -B -c "import ast,pathlib; ast.parse(pathlib.Path('Bridge/codex_light_monitor.py').read_text(encoding='utf-8')); print('OK')"

cd Firmware
pio run -j 1
```

After changing protocols, state rules, GPIOs, provisioning settings, or operating procedures, update the corresponding Chinese and English documentation. Do not commit Wi-Fi credentials, device-specific IP addresses, or other local sensitive configuration.

## License

The project is distributed under the repository's [MIT License](../LICENSE). Third-party dependencies retain their own licenses.
