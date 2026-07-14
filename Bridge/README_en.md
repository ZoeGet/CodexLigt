# CodexLight Bridge

[简体中文](README.md) | English

`codex_light_monitor.py` is the computer-side monitor for CodexLight. It watches local Codex Desktop logs and maps Codex activity to three LED states:

- `GREEN`: idle, completed, or no recent agent activity
- `RED`: thinking, writing, running tools, or taking action
- `YELLOW`: waiting for approval or explicit user input

## Log Sources

Primary source:

```text
C:\Users\<you>\.codex\sessions\YYYY\MM\DD\rollout-*.jsonl
```

The script watches appended JSONL events such as `task_started`, `function_call`, `function_call_output`, `agent_message`, and `turn_aborted`.

Secondary source:

```text
C:\Users\<you>\.codex\logs_2.sqlite
```

The script uses SQLite only as a completion hint by watching app-server diagnostic events such as `app-server event: item/completed`. It does not treat SQLite log text as a stable public API.

## State Rules

| State | Rule |
| --- | --- |
| `GREEN` | startup default, `turn_aborted`, app-server completion hint, or quiet timeout |
| `RED` | `task_started`, agent message/reasoning, tool call, tool output, or SQLite `ERROR` |
| `YELLOW` | a tool-call payload contains approval/user-input markers such as `require_escalated`, `sandbox_permissions`, `request_user_input`, `approval`, or `permission` |

The local Codex Desktop JSONL logs I inspected do not currently expose a confirmed single `task_finished` or `waiting_for_user` event. Because of that, the script returns to `GREEN` using a conservative combination of app-server completion hints and quiet timeout.

## Run

Console-only test:

```powershell
python Bridge\codex_light_monitor.py
```

Process existing recent logs from the beginning, useful for debugging state rules:

```powershell
python Bridge\codex_light_monitor.py --from-start --quiet-timeout 5
```

Send states to an ESP32 over serial:

```powershell
python Bridge\codex_light_monitor.py --serial COM5 --baud 115200
```

## Win10 Tray Background Mode

Double-click:

```text
Bridge\start_codex_light_tray.bat
```

It starts the PowerShell tray wrapper `CodexLightTray.ps1` with a hidden window, then runs `codex_light_monitor.py` in the background. On Win10, the tray icon appears in the folded notification area at the right side of the taskbar. Right-click the icon to:

- open the log folder
- restart the monitor script
- exit the background program

Logs are written to:

```text
Bridge\logs\codex_light_monitor.out.log
Bridge\logs\codex_light_monitor.err.log
```

The startup script enables both wired serial and wireless UDP by default:

```bat
set "MONITOR_ARGS=--serial auto --baud 115200 --udp --udp-port 4210"
```

To use only wired serial:

```bat
set "MONITOR_ARGS=--serial auto --baud 115200"
```

To use only wireless UDP:

```bat
set "MONITOR_ARGS=--udp --udp-port 4210"
```

Auto mode prioritizes common ESP32, Espressif USB/JTAG, CP210x, CH340/CH341, FTDI, USB Serial, CDC, and UART devices. If the serial device is disconnected, the script periodically tries to reconnect.

If auto mode picks the wrong device, replace `auto` with a fixed port, for example:

```bat
set "MONITOR_ARGS=--serial COM5 --baud 115200"
```

Console mode can also use automatic serial detection:

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200
```

Console mode can also use UDP broadcast:

```powershell
python Bridge\codex_light_monitor.py --udp --udp-port 4210
```

UDP output is one ASCII line, sent to `255.255.255.255:4210` by default. The current state is repeated every 2 seconds as a heartbeat:

```text
CODEXLIGHT/1 GREEN
CODEXLIGHT/1 RED
CODEXLIGHT/1 YELLOW
```

Serial output is one ASCII line per state change:

```text
GREEN
RED
YELLOW
```

The firmware can read complete lines from `Serial` and call the matching LED control method.

## ESP32 Wi-Fi Configuration

Wireless UDP mode needs Wi-Fi credentials in the firmware. Copy:

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

`wifi_secrets.h` is ignored by Git and will not be committed to GitHub. Without this file, the firmware still builds and works over USB serial.

## Useful Options

```text
--poll 0.5              Poll interval in seconds
--quiet-timeout 20      Fallback seconds with no activity before GREEN
--complete-grace 1.5    Delay after app-server item/completed before GREEN
--repeat                Print/send repeated identical states
```

## Notes

Serial mode and automatic serial detection require pyserial in the Python environment used to run the script:

```powershell
pip install pyserial
```

The default console monitoring mode has no third-party dependency.

UDP mode has no third-party Python dependency.

The tray wrapper uses the built-in Win10 PowerShell/.NET `NotifyIcon`, so no extra tray library is required.
