# CodexLight Implementation Status

[中文](当前实现说明.md) | English

This document is intended for future Codex conversations or developers taking over the project. It summarizes the features currently implemented in CodexLight, how they work, and the key technical decisions.

## Project Goal

CodexLight is a Codex status light based on ESP32-C3 SuperMini. The desktop side monitors local Codex Desktop logs, derives the current Codex state, and sends that state to the ESP32. The ESP32 controls three independent WS2812B LEDs.

Current state mapping:

| Light | Meaning |
| --- | --- |
| Green | Codex is idle, completed, or has had no recent activity |
| Red | Codex is thinking, writing, running tools, or taking action |
| Yellow | Codex needs approval or is waiting for explicit user input |

## Implemented Features

### Desktop Bridge

Implemented files:

```text
Bridge/codex_light_monitor.py
Bridge/CodexLightTray.ps1
Bridge/start_codex_light_tray.bat
Bridge/README.md
Bridge/README_en.md
```

Implemented capabilities:

- Monitors local Codex session JSONL logs.
- Reads Codex SQLite diagnostic logs as completion hints.
- Maps Codex activity to `GREEN` / `RED` / `YELLOW`.
- Supports USB serial output.
- Supports automatic serial scanning with `--serial auto`.
- Supports UDP broadcast output with `--udp --udp-port 4210`.
- Supports Win10 tray background mode.
- Reconnects periodically after serial disconnects.
- Repeats UDP heartbeat packets so the ESP32 can recover state after rebooting.
- Supports UDP pairing, with the random token, ESP32 MAC, and recent IP saved in `Bridge/config.local.json`.
- During normal operation, prefers unicast to the bound ESP32 IP; broadcast is used only for discovery/pairing fallback.

### ESP32 Firmware

Implemented files:

```text
Firmware/src/main.cpp
Firmware/src/led.cpp
Firmware/include/led.h
Firmware/include/config.h
Firmware/include/wifi_secrets.example.h
Firmware/platformio.ini
```

Implemented capabilities:

- Uses Arduino Framework and FastLED.
- Controls three independent WS2812B LEDs.
- Receives state commands over USB serial.
- Receives state commands over Wi-Fi UDP.
- Builds and works over USB serial even when Wi-Fi credentials are not configured.
- Lights only the matching color after receiving a valid state.
- Shows yellow after a long UDP heartbeat timeout to indicate that wireless connection may be lost.
- Stores the UDP control token in NVS, so the token does not need to be hard-coded into firmware.

## Codex Log Sources

The main state source verified locally is Codex session JSONL:

```text
C:\Users\<you>\.codex\sessions\YYYY\MM\DD\rollout-*.jsonl
```

Each line is one JSON object. Example key event:

```json
{"type":"event_msg","payload":{"type":"task_started","turn_id":"..."}}
```

The auxiliary log source is SQLite:

```text
C:\Users\<you>\.codex\logs_2.sqlite
```

SQLite is used only as a completion hint. For example, diagnostic logs can contain:

```text
app-server event: item/completed
```

Important: the local Codex Desktop logs inspected so far do not expose a confirmed stable single `task_finished` or `waiting_for_user` event. The Bridge therefore uses a combined strategy:

- `task_started` switches to red.
- Agent message/reasoning, tool calls, and tool outputs keep red active.
- Tool payloads containing approval, permission, or user-input markers switch to yellow.
- App-server completion hints or quiet timeout switch back to green.

## Bridge State Rules

Core script:

```text
Bridge/codex_light_monitor.py
```

Main rules:

| Input event | Output state |
| --- | --- |
| `event_msg.payload.type == "task_started"` | `RED` |
| `agent_message` / `agent_reasoning` | `RED` |
| `response_item.payload.type == "function_call"` | `RED` |
| Tool-call payload contains `require_escalated` / `sandbox_permissions` / `request_user_input` / `approval` / `permission` | `YELLOW` |
| `response_item.payload.type == "function_call_output"` | usually remains `RED` |
| SQLite `level == "ERROR"` | `RED` |
| `turn_aborted` | `GREEN` |
| `app-server event: item/completed` after `--complete-grace` | `GREEN` |
| No new activity after `--quiet-timeout` | `GREEN` |

## Connection Methods

### Method 1: USB Serial

Desktop command:

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200
```

Serial protocol, one ASCII state per line:

```text
GREEN
RED
YELLOW
```

Automatic serial scanning depends on pyserial:

```powershell
pip install pyserial
```

Auto mode prioritizes common ESP32/USB serial devices, including Espressif, CP210x, CH340/CH341, FTDI, USB Serial, and CDC/UART devices.

### Method 2: Wireless UDP

Desktop command:

```powershell
python Bridge\codex_light_monitor.py --udp --udp-port 4210
```

UDP is sent by default to:

```text
255.255.255.255:4210
```

UDP protocol, one ASCII line:

```text
CODEXLIGHT/1 token=<paired-token> GREEN
CODEXLIGHT/1 token=<paired-token> RED
CODEXLIGHT/1 token=<paired-token> YELLOW
```

The Bridge repeats the current state every 2 seconds as a wireless heartbeat.

First-time pairing command:

```powershell
python Bridge\codex_light_monitor.py --pair --udp-port 4210
```

After pairing succeeds, the desktop token, ESP32 MAC, and recent IP are saved to `Bridge/config.local.json`, and the ESP32 token is saved to NVS. During normal operation, the ESP32 periodically broadcasts `HELLO mac=...`; the Bridge only accepts matching-MAC HELLO packets to refresh the IP, then prefers unicast control packets.

### Method 3: USB Serial and UDP Together

Desktop command:

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200 --udp --udp-port 4210
```

The current Win10 tray startup script uses this mode by default.

## Win10 Tray Background Mode

Entry file:

```text
Bridge/start_codex_light_tray.bat
```

Double-clicking it starts:

```text
Bridge/CodexLightTray.ps1
```

Implementation details:

- Uses Windows PowerShell and .NET `System.Windows.Forms.NotifyIcon` to create a tray icon.
- Starts `codex_light_monitor.py` with a hidden window.
- Right-clicking the tray icon can open the log folder, restart the monitor script, or exit the background program.
- Does not depend on `pystray` or other third-party tray libraries.

Default arguments in `start_codex_light_tray.bat`:

```bat
set "MONITOR_ARGS=--serial auto --baud 115200 --udp --udp-port 4210"
```

For UDP only:

```bat
set "MONITOR_ARGS=--udp --udp-port 4210"
```

For serial only:

```bat
set "MONITOR_ARGS=--serial auto --baud 115200"
```

## ESP32 Firmware Protocol

Firmware entry point:

```text
Firmware/src/main.cpp
```

The firmware supports two inputs:

- `Serial`: receives raw state commands `GREEN` / `RED` / `YELLOW`.
- UDP: receives token-prefixed commands `CODEXLIGHT/1 token=<paired-token> GREEN` / `RED` / `YELLOW`.

Unpaired devices, or devices in pairing mode, accept `CODEXLIGHT/1 PAIR_SET token=<new-token>` and save the token. After pairing, UDP control packets must include the matching token.

## Wi-Fi Configuration

Wi-Fi credentials are not committed to GitHub. Usage:

1. Copy the example file:

```text
Firmware/include/wifi_secrets.example.h
```

2. Rename it to:

```text
Firmware/include/wifi_secrets.h
```

3. Fill in Wi-Fi credentials:

```cpp
#define CODEXLIGHT_WIFI_SSID "YourWiFiName"
#define CODEXLIGHT_WIFI_PASSWORD "YourWiFiPassword"
```

`.gitignore` ignores:

```text
Firmware/include/wifi_secrets.h
```

Without `wifi_secrets.h`, the firmware still builds and works over USB serial.

## Key Configuration

File:

```text
Firmware/include/config.h
```

Current configuration:

```cpp
RED_LED_PIN = 7
GREEN_LED_PIN = 6
YELLOW_LED_PIN = 5
LEDS_PER_CHANNEL = 1
DEFAULT_BRIGHTNESS = 64
SERIAL_BAUD = 115200
UDP_PORT = 4210
WIRELESS_TIMEOUT_MS = 10000
```

## Verification Commands

Desktop script syntax check:

```powershell
python -m py_compile Bridge\codex_light_monitor.py
```

Firmware build:

```powershell
cd Firmware
pio run
```

Most recent verification:

- `python -m py_compile Bridge\codex_light_monitor.py` passed.
- `pio run` passed.

## Future Work

- Add a firmware debug-output switch to confirm Wi-Fi and UDP packet status.
- Add UDP device discovery: ESP32 broadcasts `HELLO`, and the Bridge records the ESP32 IP for both unicast and broadcast output.
- Add a Bridge configuration file so users do not need to edit `.bat` directly.
- If Codex Desktop later exposes stable `task_finished` / `waiting_for_user` events, replace the current quiet-timeout inference with those events.
