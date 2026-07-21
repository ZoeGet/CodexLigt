# CodexLight

English | [简体中文](README.md) | [English Usage Guide](USAGE.en.md) | [中文使用说明](USAGE.md)

CodexLight is an ESP32-C3 status light for Codex Desktop. A Windows Bridge reads local Codex session logs, maps activity to `GREEN`, `RED`, or `YELLOW`, and sends the state to an ESP32-C3 over USB serial, LAN UDP, or both. The firmware drives three independent WS2812B LEDs for idle, working, and waiting states.

This is an independent community project and is not officially affiliated with or endorsed by OpenAI.

## Current Design

- Wi-Fi provisioning uses USB serial. The firmware no longer opens an ESP32 AP portal.
- The tray app provides `Configure WiFi`, which sends SSID/password to the device over USB.
- Wi-Fi credentials are saved to ESP32 NVS only after a successful connection. Bad credentials do not overwrite the previous working setup.
- Saved Wi-Fi connects non-blockingly at boot and keeps retrying after disconnects.
- ESP32-C3 Wi-Fi transmit power defaults to 8.5 dBm to improve wireless stability on some development boards.
- Standalone power does not depend on USB serial, so the device can run from a power bank or battery supply in wireless mode.
- Persistent `AUTO`, `WIRED`, and `WIRELESS` transport modes are supported.
- `Bridge/start_codex_light_tray.vbs` starts the tray hidden and defaults to wireless mode.

## State Mapping

| State | Codex condition | LED | GPIO |
| --- | --- | --- | --- |
| `GREEN` | Idle, task completed, or task aborted | Green | GPIO6 |
| `RED` | Reasoning, responding, running tools, or processing a task | Red | GPIO7 |
| `YELLOW` | Waiting for approval, permission, or explicit user input | Yellow | GPIO5 |
| No desktop heartbeat | No valid USB/UDP heartbeat for 6 seconds | Slow yellow blink | GPIO5 |

When the first valid desktop heartbeat arrives, the green LED blinks for two seconds as a connection indication, then the real state is shown. Completed green stays latched until the next task starts, a waiting state appears, or the connection is lost.

## Quick Start

1. Install the desktop dependency:

   ```powershell
   python -m pip install pyserial
   ```

2. Build and upload the firmware:

   ```powershell
   cd Firmware
   pio run
   pio run -t upload --upload-port COM4
   ```

3. Start the hidden tray launcher. It defaults to wireless mode:

   ```text
   Bridge\start_codex_light_tray.vbs
   ```

4. For first-time setup, connect the ESP32-C3 over USB, right-click the tray icon, choose `Configure WiFi`, and enter the router SSID/password.
5. After Wi-Fi is saved, unplug the computer USB and power the device from a power bank or battery supply. The tray controls the light over UDP.

See [USAGE.en.md](USAGE.en.md) for the full workflow.

## Wireless Operation

1. Complete `Configure WiFi` once over USB.
2. Use `Wireless only`, or start the default `start_codex_light_tray.vbs` launcher.
3. Power the ESP32-C3 and LEDs from a power bank or stable 5 V supply, without connecting USB to the computer.
4. The tray log should show:

   ```text
   SERIAL no matching serial port
   SERIAL setup skipped; using saved firmware mode.
   UDP ack from 192.168.x.x CODEXLIGHT/1 ACK ... active=WIRELESS state=...
   ```

This means there is no computer serial link and the device is working over Wi-Fi UDP.

## Standalone LED Diagnostics

| LED pattern | Meaning |
| --- | --- |
| Alternating red/yellow | No saved Wi-Fi credentials; provision over USB |
| Repeating red double-blink | Saved Wi-Fi exists, but the device is reconnecting or failed to connect |
| Slow yellow blink | Wi-Fi is connected and the device is waiting for UDP heartbeats |
| Normal red/green/yellow | Desktop state is being received |

## Hardware

Recommended parts:

- ESP32-C3 SuperMini or a compatible ESP32-C3 development board
- Three WS2812B RGB LEDs
- Stable 5 V supply, power bank, or suitable Li-ion boost converter
- USB data cable for flashing and first-time Wi-Fi provisioning

Wiring:

| Function | ESP32-C3 pin |
| --- | --- |
| Yellow LED DIN | GPIO5 |
| Green LED DIN | GPIO6 |
| Red LED DIN | GPIO7 |
| WS2812B VCC | 5 V |
| WS2812B GND | GND |

The LEDs use three independent data inputs; they are not a chained strip. The firmware uses `NEO_GRB + NEO_KHZ800`. Default brightness is `DEFAULT_BRIGHTNESS = 50` and can be configured in [Firmware/include/config.h](Firmware/include/config.h).

## Repository Layout

```text
CodexLight/
├── Bridge/        # Windows log monitor, serial/UDP sender, and tray app
├── Firmware/      # ESP32-C3 PlatformIO firmware
├── Hardware/      # BOM, schematic, PCB, Gerber, enclosure STL files
├── Docs/          # Usage and implementation notes
├── README.md      # Chinese project documentation
├── README.en.md   # English project documentation
├── USAGE.md       # Chinese usage guide
└── USAGE.en.md    # English usage guide
```

## Local Files

Do not commit local runtime state:

- `Bridge/config.local.json`: locally discovered device MAC/IP.
- `Bridge/logs/`: runtime and Wi-Fi setup logs.
- `.pio/`, `Firmware/.pio/`: PlatformIO build output.
- Wi-Fi passwords, private SSIDs, local COM ports, or temporary files.

## Verification

```powershell
python -B -m py_compile Bridge\codex_light_monitor.py
powershell -NoProfile -Command "$e=$null; [System.Management.Automation.PSParser]::Tokenize((Get-Content -LiteralPath 'Bridge\CodexLightTray.ps1' -Raw), [ref]$e) | Out-Null; if($e){$e; exit 1}else{'OK'}"
cd Firmware
pio run
```

## License

This project is licensed under the [MIT License](LICENSE). Third-party dependencies remain subject to their own licenses.
