# CodexLight

[English](README.en.md) | 简体中文

CodexLight 是一套基于 ESP32-C3 的 Codex Desktop 状态灯。电脑端 Bridge 读取本机 Codex 会话日志，将当前状态通过 USB 串口或同一局域网内的 UDP 发送给 ESP32；ESP32 控制三颗独立 WS2812B 灯珠显示工作、等待和完成状态。

本项目是社区开源项目，与 OpenAI 无官方关联或背书。

## 功能

- USB 串口和 Wi-Fi UDP 两种通信方式。
- `AUTO`、`WIRED`、`WIRELESS` 三种可持久化通信模式。
- 手机连接 ESP32 热点后，通过网页完成 2.4 GHz Wi-Fi 配网。
- 配网流程为非阻塞实现，未联网时仍可使用 USB 串口。
- Bridge 每 2 秒发送一次状态心跳，固件在 6 秒未收到心跳后显示断开状态。
- 建立电脑端连接时，绿灯闪烁 2 秒，然后显示真实 Codex 状态。
- 支持 Windows 托盘后台运行。
- 提供 BOM、PCB、原理图和 3D 打印外壳文件。

## 状态定义

| 状态 | Codex 状态 | LED | GPIO |
| --- | --- | --- | --- |
| `GREEN` | 空闲、任务完成或任务中止 | 绿灯 | GPIO6 |
| `RED` | 思考、回复、执行工具或处理任务 | 红灯 | GPIO7 |
| `YELLOW` | 等待批准、权限确认或用户输入 | 黄灯 | GPIO5 |
| 未连接 | 6 秒内未收到有效电脑端心跳 | 黄灯每秒闪烁一次 | GPIO5 |

首次收到有效心跳时，GPIO6 绿灯会闪烁 2 秒作为连接提示。动画结束后，只点亮当前真实状态对应的灯。任务完成后，绿色状态会持续锁存，直到下一次任务开始、进入等待状态或连接断开。

## 硬件

### 推荐器件

- ESP32-C3 SuperMini 或兼容的 ESP32-C3 开发板
- 3 颗 WS2812B RGB LED
- 稳定的 5 V 电源
- USB 数据线

### 接线

| 功能 | ESP32-C3 引脚 |
| --- | --- |
| 黄灯 DIN | GPIO5 |
| 绿灯 DIN | GPIO6 |
| 红灯 DIN | GPIO7 |
| WS2812B VCC | 5 V |
| WS2812B GND | GND |

三颗 WS2812B 是三路独立数据输入，不是串联灯带。固件与参考硬件一致，使用 `NEO_GRB + NEO_KHZ800`，颜色参数按标准 RGB 顺序传入。如果使用不同批次灯珠后颜色不正确，请修改 [Firmware/src/led.cpp](Firmware/src/led.cpp) 中的色序。

当前全局亮度为 `25/255`，在 [Firmware/include/config.h](Firmware/include/config.h) 的 `DEFAULT_BRIGHTNESS` 中配置。

硬件资料位于：

- `Hardware/BOM/BOM.xlsx`：元器件物料清单
- `Hardware/Schematic/Schematic1.pdf`：原理图
- `Hardware/PCB/Source/CodexLight.epro2`：PCB 源工程
- `Hardware/PCB/Gerber/CodexLight_PCB_Gerber.zip`：Gerber 制造文件
- `Hardware/Enclosure/`：上下壳 STL 文件

## 项目结构

```text
CodexLight/
├─ Bridge/                 # 电脑端日志监听、串口/UDP发送和Windows托盘程序
├─ Firmware/               # ESP32-C3 PlatformIO固件
├─ Hardware/
│  ├─ BOM/                 # 元器件物料清单
│  ├─ Schematic/           # 电路原理图
│  ├─ PCB/                 # PCB源工程和Gerber制造文件
│  └─ Enclosure/           # 3D打印上下壳
├─ Docs/                   # 使用说明和实现细节
├─ README.md               # 中文说明
├─ README.en.md            # English documentation
└─ LICENSE                 # MIT License
```

## 环境要求

### 电脑端

- Windows 10/11
- Python 3.9 或更高版本
- Codex Desktop
- 有线模式需要 `pyserial`

```powershell
python -m pip install pyserial
```

### 固件端

- PlatformIO Core 或 VS Code PlatformIO IDE
- Espressif32 Platform
- Arduino Framework

PlatformIO 会自动安装：

- `Adafruit NeoPixel`
- `WiFiManager`

## 编译和烧录

在 PowerShell 中执行：

```powershell
cd C:\path\to\CodexLight\Firmware
pio run
pio run -t upload --upload-port COM4
```

将 `COM4` 替换为 ESP32-C3 实际端口。固件使用 USB CDC，串口波特率固定为 `115200`。

查看启动信息：

```powershell
pio device monitor --port COM4
```

正常启动时应看到类似内容：

```text
CODEXLIGHT READY
STATUS mode=WIRED active=NONE wifi=DISCONNECTED
```

退出串口监视器使用 `Ctrl+C`。同一串口不能同时被 PlatformIO Monitor 和 Bridge 占用。

## Wi-Fi 配网

### 首次配网

没有可用 Wi-Fi 凭据时，ESP32 会开启配置热点：

```text
SSID: CodexLight-XXXX
Password: 123456789
URL: http://192.168.4.1
```

操作步骤：

1. 使用手机连接 `CodexLight-XXXX` 热点。
2. 如果系统没有自动打开配置页，在浏览器访问 `http://192.168.4.1`。
3. 选择 ESP32 和电脑都能连接的 2.4 GHz Wi-Fi。
4. 输入该 Wi-Fi 的密码并保存。
5. ESP32 连接成功后，配置热点会关闭；以后上电会自动连接已保存的网络。

ESP32-C3 只支持 2.4 GHz Wi-Fi。无线控制要求电脑和 ESP32 位于同一局域网，且路由器没有启用客户端隔离。

### 修改配置热点

编辑 [Firmware/include/config.h](Firmware/include/config.h)：

```cpp
constexpr const char* CONFIG_AP_SSID_PREFIX = "CodexLight";
constexpr const char* CONFIG_AP_PASSWORD = "123456789";
```

AP 密码至少需要 8 个字符。修改后必须重新编译并烧录固件。

### 清除或重新配置 Wi-Fi

打开串口监视器并发送：

```text
WIFI_CONFIG
CLEAR_WIFI
```

- `WIFI_CONFIG`：强制打开配网热点。
- `CLEAR_WIFI`：删除已保存的 Wi-Fi，并打开配网热点。

## 通信模式

| 模式 | 行为 |
| --- | --- |
| `WIRED` | 只接受 USB 串口状态，当前默认值 |
| `WIRELESS` | 只接受同一局域网内的 UDP 状态 |
| `AUTO` | 同时接受串口和 UDP；有效串口心跳优先 |

运行时可通过串口切换：

```text
MODE WIRED
MODE WIRELESS
MODE AUTO
STATUS
```

模式会保存到 ESP32 NVS，重新烧录固件通常不会清除该设置。Bridge 仅启用串口时自动发送 `MODE WIRED`；串口和 UDP 同时启用时自动发送 `MODE AUTO`，因此日常运行不需要手动清除 NVS。

固件默认模式可在 [Firmware/include/config.h](Firmware/include/config.h) 中修改：

```cpp
constexpr const char* DEFAULT_TRANSPORT_MODE = "WIRED";
```

## 启动 Bridge

回到项目根目录执行。

### USB 有线模式

```powershell
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200
```

也可以自动选择常见 ESP32 串口：

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200
```

如果电脑连接了多个串口设备，建议显式指定 `COM4`。

### Wi-Fi 无线模式

固件当前默认模式是 `WIRED`。第一次使用纯无线模式前，先通过 USB 串口发送并保存：

```text
MODE WIRELESS
```

也可以发送 `MODE AUTO`，让固件同时接受无线和后续可能出现的有线心跳。模式保存在 NVS，设置一次后重新上电仍然有效。

```powershell
python Bridge\codex_light_monitor.py --udp --udp-port 4210
```

Bridge 初始使用 UDP 广播发现设备。收到 ESP32 的 `HELLO` 后，会把设备 MAC 和最近 IP 保存到未纳入 Git 的 `Bridge/config.local.json`，随后优先使用单播。

### 同时启用两种方式

```powershell
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200 --udp --udp-port 4210
```

Bridge 检测到串口和 UDP 同时启用时会自动发送 `MODE AUTO`。固件随后根据有效心跳选择传输；两种心跳同时有效时有线优先。

### Windows 托盘模式

双击：

```text
Bridge\start_codex_light_tray.bat
```

默认同时启动自动串口和 UDP。托盘菜单可以打开日志目录、重启 Bridge 或退出。参数可在批处理文件中的 `MONITOR_ARGS` 修改。

## 串口命令

| 命令 | 作用 |
| --- | --- |
| `GREEN` | 更新有线状态为绿色并刷新心跳 |
| `RED` | 更新有线状态为红色并刷新心跳 |
| `YELLOW` | 更新有线状态为黄色并刷新心跳 |
| `PING` | 刷新有线心跳，回复 `PONG` |
| `STATUS` | 输出模式、活动传输、Wi-Fi 和 IP |
| `MODE WIRED` | 只使用串口并保存到 NVS |
| `MODE WIRELESS` | 只使用 UDP 并保存到 NVS |
| `MODE AUTO` | 同时接受串口和 UDP 并保存到 NVS |
| `WIFI_CONFIG` | 打开 Wi-Fi 配网热点 |
| `CLEAR_WIFI` | 清除 Wi-Fi 配置并打开热点 |

## 通信协议

串口协议为每行一个 ASCII 命令：

```text
GREEN\n
RED\n
YELLOW\n
```

UDP 状态协议：

```text
CODEXLIGHT/1 GREEN
CODEXLIGHT/1 RED
CODEXLIGHT/1 YELLOW
```

ESP32 每 2 秒广播发现包：

```text
CODEXLIGHT/1 HELLO mac=AA:BB:CC:DD:EE:FF mode=WIRELESS
```

默认 UDP 端口为 `4210`，心跳间隔为 2 秒，ESP32 连接超时为 6 秒。

## 状态判断规则

Bridge 监听 `~/.codex/sessions` 下的 Codex JSONL 会话日志：

- `task_started`、思考、消息、工具调用和工具输出：`RED`
- 需要批准、权限或用户输入的工具调用：`YELLOW`
- `task_complete` 或 `turn_aborted`：`GREEN`

活动任务不会因为普通 `item/completed` 或短暂无日志而提前变绿。进入 `GREEN` 后，Bridge 会持续发送绿色心跳，固件保持绿灯，直到下一次状态或断开状态覆盖它。

## 故障排查

### Bridge 显示已连接，但黄灯仍闪烁

1. 确认烧录的是最新固件。
2. 确认 Bridge 和串口监视器没有同时占用 COM 端口。
3. 使用显式端口运行 Bridge，例如 `--serial COM4`。
4. 打开串口监视器发送 `STATUS`，确认模式为 `WIRED` 或 `AUTO`。
5. 等待连接动画结束；第一次收到心跳时绿灯会先闪烁 2 秒。

### 手机无法连接配置热点

- 确认密码是 `123456789`，或检查 `config.h` 中修改后的密码。
- 断开手机的自动 Wi-Fi 切换或移动数据辅助功能。
- 手动访问 `http://192.168.4.1`。
- 通过串口发送 `CLEAR_WIFI` 后重新连接热点。

### 已配网但无线模式无响应

- 确认电脑和 ESP32 连接同一个 2.4 GHz 局域网。
- 确认防火墙允许 Python 使用 UDP 4210。
- 确认路由器没有开启 AP/客户端隔离。
- 发送 `MODE WIRELESS` 或 `MODE AUTO`。
- 删除 `Bridge/config.local.json` 可让 Bridge 重新发现设备 IP。

### 颜色不正确

当前硬件使用 `NEO_GRB` 色序。如果灯珠批次不同，请在 `Firmware/src/led.cpp` 中尝试 `NEO_RGB` 或对应色序后重新烧录。

### PlatformIO 提示多个 Core

该提示来自电脑中同时安装了多个 PlatformIO Core，不是固件错误。按照 PlatformIO 输出的 troubleshooting 链接移除旧版本即可。

## 开发验证

```powershell
# Bridge 语法检查，不写 __pycache__
python -B -c "import ast,pathlib; ast.parse(pathlib.Path('Bridge/codex_light_monitor.py').read_text(encoding='utf-8')); print('OK')"

# 固件构建
cd Firmware
pio run -j 1
```

## 文档

- [Bridge 中文说明](Bridge/README.md)
- [Bridge English Guide](Bridge/README_en.md)
- [重构后使用说明](Docs/重构后使用说明.md)
- [当前实现说明](Docs/当前实现说明.md)
- [Implementation Status](Docs/IMPLEMENTATION_STATUS.md)

## 安全说明

当前 UDP 控制协议没有加密或鉴权，只应在可信局域网内使用。请修改默认 AP 密码，并避免把设备暴露到公共或不受信任网络。

## 贡献

欢迎提交 Issue 或 Pull Request。提交代码前请确认：

- Bridge 脚本可以通过 Python 语法检查。
- 固件可以通过 `pio run` 构建。
- 新行为已经同步更新中英文文档。
- 不提交 Wi-Fi 密码、设备 IP 或其他本地配置。

## 许可证

本项目使用 [MIT License](LICENSE)。第三方依赖分别遵循其自身许可证。
