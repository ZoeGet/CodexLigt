# CodexLight 使用说明

[English](USAGE.en.md) | 简体中文 | [返回项目主页](README.md)

本文集中说明 CodexLight 的固件编译与烧录、手机 AP 配网、有线与无线连接、Windows 托盘、通信命令、状态判断、故障排查和开发验证。

## 使用前准备

电脑端需要 Windows 10/11、Python 3.9 或更高版本和 Codex Desktop。有线模式还需要 `pyserial`：

```powershell
python -m pip install pyserial
```

固件端需要 PlatformIO Core 或 VS Code PlatformIO IDE。PlatformIO 会自动安装 `Adafruit NeoPixel` 和 `WiFiManager`。

## 编译和烧录

在 PowerShell 中执行：

```powershell
cd C:\path\to\CodexLight\Firmware
pio run -j 1
pio run -t upload --upload-port COM4
```

将 `COM4` 替换为 ESP32-C3 实际端口。固件使用 USB CDC，串口波特率为 `115200`。

查看启动信息：

```powershell
pio device monitor --port COM4 --baud 115200
```

正常启动时应看到类似内容：

```text
CODEXLIGHT READY
STATUS mode=WIRED active=NONE wifi=DISCONNECTED
```

退出串口监视器使用 `Ctrl+C`。PlatformIO Monitor 和 Bridge 不能同时占用同一串口。

## Wi-Fi 配网

### 首次配网

没有可用 Wi-Fi 凭据时，ESP32 会开启配置热点：

```text
SSID: CodexLight-XXXX
Password: 123456789
URL: http://192.168.4.1
```

1. 使用手机连接 `CodexLight-XXXX`。
2. 如果系统没有自动打开配置页，在浏览器访问 `http://192.168.4.1`。
3. 选择 ESP32 和电脑都能连接的 2.4 GHz Wi-Fi。
4. 输入该 Wi-Fi 的密码并保存。
5. ESP32 连接成功后关闭配置热点，以后上电会自动连接已保存网络。

ESP32-C3 只支持 2.4 GHz Wi-Fi。无线控制要求电脑和 ESP32 位于同一局域网，并且路由器没有开启 AP 或客户端隔离。

### 修改配置热点

编辑 [Firmware/include/config.h](Firmware/include/config.h)：

```cpp
constexpr const char* CONFIG_AP_SSID_PREFIX = "CodexLight";
constexpr const char* CONFIG_AP_PASSWORD = "123456789";
```

AP 密码至少需要 8 个字符。修改后必须重新编译并烧录固件。

### 清除或重新配置 Wi-Fi

通过串口发送：

```text
WIFI_CONFIG
CLEAR_WIFI
```

- `WIFI_CONFIG`：强制打开配网热点。
- `CLEAR_WIFI`：删除已保存的 Wi-Fi 凭据并打开配网热点。

## 通信模式

| 模式 | 行为 |
| --- | --- |
| `WIRED` | 只接受 USB 串口状态，当前默认值 |
| `WIRELESS` | 只接受同一局域网内的 UDP 状态 |
| `AUTO` | 同时接受串口和 UDP，有效串口心跳优先 |

运行时可通过串口切换：

```text
MODE WIRED
MODE WIRELESS
MODE AUTO
STATUS
```

模式保存在 ESP32 NVS 中，普通固件烧录通常不会清除。固件默认模式位于 [Firmware/include/config.h](Firmware/include/config.h)：

```cpp
constexpr const char* DEFAULT_TRANSPORT_MODE = "WIRED";
```

需要完全擦除 NVS 时：

```powershell
cd Firmware
pio run -t erase --upload-port COM4
pio run -t upload --upload-port COM4
```

## 启动 Bridge

以下命令均在项目根目录执行。

### USB 有线模式

```powershell
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200
```

自动选择常见 ESP32 串口：

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200
```

如果电脑连接了多个串口设备，建议显式指定端口。

### Wi-Fi 无线模式

第一次使用纯无线模式前，将固件保存为 `WIRELESS` 或 `AUTO`：

```text
MODE WIRELESS
```

然后关闭串口监视器并运行：

```powershell
python Bridge\codex_light_monitor.py --udp --udp-port 4210
```

Bridge 初始使用 UDP 广播发现设备。收到 ESP32 的 `HELLO` 后，会把设备 MAC 和最近 IP 保存到已被 Git 忽略的 `Bridge/config.local.json`，随后优先使用单播。

### 同时启用有线和无线

```powershell
python Bridge\codex_light_monitor.py --serial COM4 --baud 115200 --udp --udp-port 4210
```

Bridge 会发送并确认 `MODE AUTO`。两种心跳同时有效时，固件优先使用有线连接。

## Windows 托盘模式

双击：

```text
Bridge\start_codex_light_tray.bat
```

默认以 `AUTO` 模式启动。也可以预选模式：

```powershell
Bridge\start_codex_light_tray.bat auto
Bridge\start_codex_light_tray.bat wired
Bridge\start_codex_light_tray.bat wireless
```

托盘右键菜单的 `Connection mode` 可以直接切换：

- `Auto (wired + wireless)`
- `Wired only`
- `Wireless only`

切换模式时 Bridge 会自动重启。无线模式的状态只通过 UDP 发送；USB 可用时，Bridge 会先通过串口保存 `MODE WIRELESS`，收到确认后立即释放串口。没有 USB 或未安装 `pyserial` 时，无线模式使用 ESP32 已保存的模式。

批处理顶部可以统一配置：

```bat
set "SERIAL_PORT=auto"
set "SERIAL_BAUD=115200"
set "UDP_PORT=4210"
```

## LED 行为

- 未收到有效电脑心跳：GPIO5 黄灯按 1 秒完整周期闪烁。
- 首次建立电脑连接：GPIO6 绿灯闪烁 2 秒。
- Codex 思考、回复或执行工具：GPIO7 红灯常亮。
- 等待批准、权限或用户输入：GPIO5 黄灯常亮。
- `task_complete` 或 `turn_aborted`：GPIO6 绿灯常亮。
- 当前传输超过 6 秒没有心跳：恢复黄灯闪烁。

Bridge 会保持批准对应的 `YELLOW`，普通并行工具事件和诊断错误不能覆盖它，直到对应批准调用返回。任务完成后的 `GREEN` 会保持到下一次任务开始、进入等待状态或连接断开。

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
CODEXLIGHT/1 PING
```

ESP32 每 2 秒广播发现包：

```text
CODEXLIGHT/1 HELLO mac=AA:BB:CC:DD:EE:FF mode=WIRELESS
```

默认 UDP 端口为 `4210`，Bridge 心跳间隔为 2 秒，ESP32 连接超时为 6 秒。

## 状态判断规则

Bridge 监听 `~/.codex/sessions` 下的 Codex JSONL 会话日志：

- `task_started`、思考、消息、工具调用和工具输出：`RED`
- 需要批准、权限或用户输入的工具调用：`YELLOW`
- `task_complete` 或 `turn_aborted`：`GREEN`

Bridge 只向 ESP32 发送颜色状态，不发送 Codex 消息正文、工具输出、API Key 或登录令牌。

## 故障排查

### Bridge 已连接，但黄灯仍闪烁

1. 确认烧录的是最新固件。
2. 关闭占用同一 COM 端口的 PlatformIO Monitor。
3. 使用显式端口运行 Bridge，例如 `--serial COM4`。
4. 发送 `STATUS`，确认模式为 `WIRED` 或 `AUTO`。
5. 等待 2 秒绿灯连接动画结束。

### 等待批准时黄灯不常亮

- 重启 Bridge，确保运行的是最新脚本。
- PowerShell 应显示 `YELLOW approval_needed:<tool>`。
- 托盘模式下选择 `Restart monitor`。

### 手机无法连接配置热点

- 确认密码是 `123456789`，或检查 `config.h` 中的修改值。
- 暂时关闭手机自动 Wi-Fi 切换或移动数据辅助。
- 手动访问 `http://192.168.4.1`。
- 通过串口发送 `CLEAR_WIFI` 后重试。

### 已配网但无线模式无响应

- 确认电脑和 ESP32 位于同一个 2.4 GHz 局域网。
- 确认防火墙允许 Python 使用 UDP 4210。
- 确认路由器没有开启 AP 或客户端隔离。
- 保存 `MODE WIRELESS` 或 `MODE AUTO`。
- 删除 `Bridge/config.local.json`，让 Bridge 重新发现设备。

### LED 颜色不正确

当前硬件使用 `NEO_GRB`。更换灯珠批次后，应确认色序、DIN 方向、GPIO 连通性和焊接质量。

### PlatformIO 提示多个 Core

这表示电脑中同时安装了多个 PlatformIO Core，不是固件错误。按照 PlatformIO 输出的 troubleshooting 链接移除旧版本。

## 安全说明

- UDP 控制协议没有加密和鉴权，只应在可信局域网使用。
- 建议修改公开的默认 AP 密码。
- 不要提交 Wi-Fi 密码、设备 IP 或其他本地配置。
- `Bridge/config.local.json`、`Bridge/logs/` 和 `Firmware/include/wifi_secrets.h` 已被 Git 忽略。

## 开发验证

```powershell
# Bridge 语法检查，不写入 __pycache__
python -B -c "import ast,pathlib; ast.parse(pathlib.Path('Bridge/codex_light_monitor.py').read_text(encoding='utf-8')); print('OK')"

# PowerShell 托盘脚本语法检查
powershell -NoProfile -Command "$e=$null; [System.Management.Automation.Language.Parser]::ParseFile((Resolve-Path 'Bridge\CodexLightTray.ps1'),[ref]$null,[ref]$e) | Out-Null; if($e.Count){$e; exit 1}else{'OK'}"

# 固件构建
cd Firmware
pio run -j 1
```

更多架构和维护信息见 [Docs/使用与实现说明.md](Docs/使用与实现说明.md)。
