# CodexLight Usage Guide

English | [简体中文](USAGE.md) | [Project Home](README.en.md)

This guide covers firmware build and upload, phone-based AP provisioning, wired and wireless operation, Windows tray control, commands, state detection, troubleshooting, and development verification.

## Prerequisites

The desktop side requires Windows 10/11, Python 3.9 or newer, and Codex Desktop. Wired operation also requires `pyserial`:

```powershell
python -m pip install pyserial
```

Firmware development requires PlatformIO Core or the VS Code PlatformIO IDE. PlatformIO installs `Adafruit NeoPixel` and `WiFiManager` automatically.

## Build and Upload

Run from PowerShell:

```powershell
cd C:\path\to\CodexLight\Firmware
pio run -j 1
pio run -t upload --upload-port COM4
```

Replace `COM4` with the actual ESP32-C3 port. Firmware uses USB CDC at `115200` baud.

Open the serial monitor:

```powershell
pio device monitor --port COM4 --baud 115200
```

Expected startup output:

```text
CODEXLIGHT READY
STATUS mode=WIRED active=NONE wifi=DISCONNECTED
```

Exit with `Ctrl+C`. PlatformIO Monitor and the Bridge cannot own the same COM port simultaneously.

## Wi-Fi Provisioning

### First-Time Setup

When no usable credentials exist, the ESP32 opens:

```text
SSID: CodexLight-XXXX
Password: 123456789
URL: http://192.168.4.1
```

1. Connect the phone to `CodexLight-XXXX`.
2. If the captive portal does not open, browse to `http://192.168.4.1`.
3. Select a 2.4 GHz Wi-Fi network available to both the ESP32 and computer.
4. Enter its password and save.
5. The ESP32 closes the provisioning AP after connecting and reconnects automatically on later boots.

The ESP32-C3 supports 2.4 GHz Wi-Fi only. Wireless control requires the computer and ESP32 on the same LAN without AP or client isolation.

### Change the Provisioning AP

Edit [Firmware/include/config.h](Firmware/include/config.h):

```cpp
constexpr const char* CONFIG_AP_SSID_PREFIX = "CodexLight";
constexpr const char* CONFIG_AP_PASSWORD = "123456789";
```

The AP password must contain at least eight characters. Rebuild and upload after changing it.

### Clear or Reconfigure Wi-Fi

Send over serial:

```text
WIFI_CONFIG
CLEAR_WIFI
```

- `WIFI_CONFIG` forces the provisioning AP open.
- `CLEAR_WIFI` removes saved credentials and opens the provisioning AP.

## Transport Modes

| Mode | Behavior |
| --- | --- |
| `WIRED` | Accept USB serial only; current default |
| `WIRELESS` | Accept UDP on the same LAN only |
| `AUTO` | Accept serial and UDP; a fresh serial heartbeat has priority |

Runtime commands:

```text
MODE WIRED
MODE WIRELESS
MODE AUTO
STATUS
```

The mode is stored in ESP32 NVS and normally survives firmware uploads. The source default is configured in [Firmware/include/config.h](Firmware/include/config.h):

```cpp
constexpr const char* DEFAULT_TRANSPORT_MODE = "WIRED";
```

For a complete NVS erase:

```powershell
cd Firmware
pio run -t erase --upload-port COM4
pio run -t upload --upload-port COM4
```

## Run the Bridge

Run the following commands from the repository root.

### Wired USB

```powershell
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200
```

Automatic ESP32 serial selection:

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200
```

Use an explicit port when multiple serial devices are attached.

### Wireless UDP

Before the first wireless-only session, save `WIRELESS` or `AUTO` in firmware:

```text
MODE WIRELESS
```

Then close the serial monitor and run:

```powershell
python Bridge\codex_light_monitor.py --udp --udp-port 4210
```

The Bridge initially uses UDP broadcast discovery. After receiving an ESP32 `HELLO`, it stores the MAC and recent IP in the Git-ignored `Bridge/config.local.json`, then prefers unicast.

### Enable Both Transports

```powershell
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200 --udp --udp-port 4210
```

The Bridge sends and confirms `MODE AUTO`. Firmware prefers wired traffic when both heartbeat sources are fresh.

## Windows Tray Mode

Double-click:

```text
Bridge\start_codex_light_tray.bat
```

The default mode is `AUTO`. A startup mode can also be selected explicitly:

```powershell
Bridge\start_codex_light_tray.bat auto
Bridge\start_codex_light_tray.bat wired
Bridge\start_codex_light_tray.bat wireless
```

The tray's `Connection mode` submenu switches between:

- `Auto (wired + wireless)`
- `Wired only`
- `Wireless only`

The Bridge restarts automatically when the mode changes. Wireless state traffic uses UDP only; when USB is available, the Bridge first saves `MODE WIRELESS` over serial and releases the port after acknowledgement. Without USB or `pyserial`, wireless mode uses the firmware's saved mode.

Configure the batch file at the top:

```bat
set "SERIAL_PORT=auto"
set "SERIAL_BAUD=115200"
set "UDP_PORT=4210"
```

## LED Behavior

- No valid desktop heartbeat: GPIO5 yellow blinks with a one-second full period.
- First desktop connection: GPIO6 green blinks for two seconds.
- Codex reasoning, responding, or running tools: GPIO7 red stays on.
- Waiting for approval, permission, or user input: GPIO5 yellow stays on.
- `task_complete` or `turn_aborted`: GPIO6 green stays on.
- No heartbeat on the active transport for six seconds: returns to blinking yellow.

The Bridge latches approval-related `YELLOW` against parallel tool events and diagnostics until the matching approval call returns. Completion-related `GREEN` remains active until a new task starts, a waiting state begins, or the connection is lost.

## Serial Commands

| Command | Effect |
| --- | --- |
| `GREEN` | Set wired state to green and refresh heartbeat |
| `RED` | Set wired state to red and refresh heartbeat |
| `YELLOW` | Set wired state to yellow and refresh heartbeat |
| `PING` | Refresh wired heartbeat and reply with `PONG` |
| `STATUS` | Print mode, active transport, Wi-Fi state, and IP |
| `MODE WIRED` | Use serial only and persist in NVS |
| `MODE WIRELESS` | Use UDP only and persist in NVS |
| `MODE AUTO` | Accept serial and UDP and persist in NVS |
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
CODEXLIGHT/1 PING
```

The ESP32 broadcasts discovery every two seconds:

```text
CODEXLIGHT/1 HELLO mac=AA:BB:CC:DD:EE:FF mode=WIRELESS
```

The default UDP port is `4210`, the Bridge heartbeat interval is two seconds, and the firmware link timeout is six seconds.

## State Detection

The Bridge monitors Codex JSONL session logs under `~/.codex/sessions`:

- `task_started`, reasoning, messages, tool calls, and tool outputs: `RED`
- Tool calls requiring approval, permission, or user input: `YELLOW`
- `task_complete` or `turn_aborted`: `GREEN`

The Bridge sends only color states to the ESP32, never Codex message text, tool output, API keys, or login tokens.

## Troubleshooting

### The Bridge connects but yellow keeps blinking

1. Confirm that the latest firmware is uploaded.
2. Close PlatformIO Monitor before starting the Bridge.
3. Use an explicit port such as `--serial COM4`.
4. Send `STATUS` and confirm `WIRED` or `AUTO`.
5. Allow the two-second green connection animation to finish.

### Yellow does not stay on for approval

- Restart the Bridge to load the latest script.
- PowerShell should print `YELLOW approval_needed:<tool>`.
- In tray mode, choose `Restart monitor`.

### A phone cannot join the provisioning AP

- Confirm the password is `123456789`, or check the modified value in `config.h`.
- Temporarily disable automatic Wi-Fi switching or mobile-data assistance.
- Open `http://192.168.4.1` manually.
- Send `CLEAR_WIFI` over serial and retry.

### Wi-Fi is configured but wireless control does not work

- Put the computer and ESP32 on the same 2.4 GHz LAN.
- Allow Python to use UDP port 4210 through the firewall.
- Disable AP or client isolation on the router.
- Save `MODE WIRELESS` or `MODE AUTO`.
- Delete `Bridge/config.local.json` to force rediscovery.

### LED colors are incorrect

The current hardware uses `NEO_GRB`. For another LED batch, verify pixel order, DIN direction, GPIO continuity, and soldering.

### PlatformIO reports multiple Core installations

This warning means multiple PlatformIO Core versions are installed. It is not a firmware error. Follow PlatformIO's troubleshooting link and remove the obsolete installation.

## Security

- UDP control is neither encrypted nor authenticated; use it only on a trusted LAN.
- Change the public default provisioning AP password.
- Do not commit Wi-Fi passwords, device IP addresses, or other local configuration.
- `Bridge/config.local.json`, `Bridge/logs/`, and `Firmware/include/wifi_secrets.h` are Git-ignored.

## Development Verification

```powershell
# Bridge syntax check without writing __pycache__
python -B -c "import ast,pathlib; ast.parse(pathlib.Path('Bridge/codex_light_monitor.py').read_text(encoding='utf-8')); print('OK')"

# PowerShell tray script syntax check
powershell -NoProfile -Command "$e=$null; [System.Management.Automation.Language.Parser]::ParseFile((Resolve-Path 'Bridge\CodexLightTray.ps1'),[ref]$null,[ref]$e) | Out-Null; if($e.Count){$e; exit 1}else{'OK'}"

# Firmware build
cd Firmware
pio run -j 1
```

For architecture and maintenance details, see [Docs/USAGE_AND_IMPLEMENTATION.md](Docs/USAGE_AND_IMPLEMENTATION.md).
