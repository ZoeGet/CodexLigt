# CodexLight Firmware

[项目主页](../README.md) | [English section](#english)

这是 ESP32-C3 端的 PlatformIO Arduino 固件，负责 Wi-Fi 配网、串口/UDP 心跳接收、通信模式持久化和三路 WS2812B 状态显示。

## 构建环境

```ini
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
```

依赖由 PlatformIO 自动安装：

- `Adafruit NeoPixel`
- `WiFiManager`

## 构建与烧录

```powershell
cd Firmware
pio run -j 1
pio run -t upload --upload-port COM4
pio device monitor --port COM4
```

正常启动输出：

```text
CODEXLIGHT READY
STATUS mode=WIRED active=NONE wifi=DISCONNECTED
```

## 主要配置

编辑 `include/config.h`：

| 配置 | 当前值 | 说明 |
| --- | --- | --- |
| `YELLOW_LED_PIN` | `5` | 黄灯数据引脚 |
| `GREEN_LED_PIN` | `6` | 绿灯数据引脚 |
| `RED_LED_PIN` | `7` | 红灯数据引脚 |
| `DEFAULT_BRIGHTNESS` | `25` | NeoPixel 全局亮度，与参考项目一致 |
| `SERIAL_BAUD` | `115200` | USB CDC 串口波特率 |
| `UDP_PORT` | `4210` | UDP 监听和发现端口 |
| `LINK_TIMEOUT_MS` | `6000` | 心跳断开判定 |
| `CONFIG_AP_SSID_PREFIX` | `CodexLight` | 配网热点前缀 |
| `CONFIG_AP_PASSWORD` | `123456789` | 配网热点密码 |
| `DEFAULT_TRANSPORT_MODE` | `WIRED` | 未保存模式时的默认值 |

修改 AP 密码时至少使用 8 个字符。

## 配网流程

固件使用非阻塞 WiFiManager。没有可用凭据时开启：

```text
SSID: CodexLight-XXXX
Password: 123456789
IP: 192.168.4.1
```

手机连接后选择 2.4 GHz Wi-Fi。凭据由 WiFiManager 保存，重新上电会自动连接。配网期间主循环仍持续处理 USB 串口和 LED 状态。

## 通信模式

- `WIRED`：只接受串口心跳。
- `WIRELESS`：只接受 UDP 心跳。
- `AUTO`：同时接受两者，串口优先。

模式通过 Preferences 保存到 NVS，普通固件烧录不会清除。串口命令：

```text
MODE WIRED
MODE WIRELESS
MODE AUTO
STATUS
```

## 串口命令

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

`PING` 回复 `PONG`。`WIFI_CONFIG` 打开配网热点；`CLEAR_WIFI` 清除 Wi-Fi 后打开热点。

## LED 驱动

三颗 WS2812B 分别由独立的 `Adafruit_NeoPixel` 对象驱动。当前色序与参考项目一致，为 `NEO_GRB + NEO_KHZ800`。颜色调用保持标准 RGB 顺序：红色 `(255,0,0)`、绿色 `(0,255,0)`、黄色 `(255,255,0)`。切换状态时固件会给三路分别发送数据，明确关闭另外两颗灯。

## NVS 和擦除

通信模式和 Wi-Fi 凭据存储在非易失区域。需要完全恢复出厂状态时，先关闭串口监视器和 Bridge，然后执行：

```powershell
pio run -t erase --upload-port COM4
pio run -t upload --upload-port COM4
```

通常无需整片擦除：使用 `CLEAR_WIFI` 清 Wi-Fi，使用 `MODE WIRED`、`MODE WIRELESS` 或 `MODE AUTO` 修改通信模式即可。

## 文件

- `src/main.cpp`：通信、心跳、模式和状态机
- `src/config_portal.cpp`：WiFiManager 非阻塞配网
- `src/led.cpp`：三路 NeoPixel 驱动
- `include/config.h`：用户可修改配置
- `platformio.ini`：构建、依赖、USB CDC 和监视器配置

## English

This PlatformIO Arduino firmware targets the ESP32-C3 and implements non-blocking Wi-Fi provisioning, USB serial and UDP heartbeats, persistent transport modes, and three independent WS2812B status LEDs.

Build and upload:

```powershell
cd Firmware
pio run -j 1
pio run -t upload --upload-port COM4
pio device monitor --port COM4
```

Configuration is in `include/config.h`. The current defaults are GPIO5 yellow, GPIO6 green, GPIO7 red, NeoPixel brightness 25, `NEO_GRB` pixel order, 115200 baud, UDP port 4210, a 6-second link timeout, AP password `123456789`, and `WIRED` transport mode.

The selected transport mode and Wi-Fi credentials are stored in NVS. Use `CLEAR_WIFI` to remove Wi-Fi credentials, `MODE ...` to change transport mode, or `pio run -t erase --upload-port COM4` for a complete flash erase.

The firmware is covered by the repository's [MIT License](../LICENSE).
