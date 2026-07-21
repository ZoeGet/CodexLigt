# CodexLight

[English](README.en.md) | 简体中文 | [使用说明](USAGE.md) | [English Usage Guide](USAGE.en.md)

CodexLight 是一套基于 ESP32-C3 的 Codex Desktop 状态灯。Windows 端 Bridge 读取本机 Codex 会话日志，把状态转换为 `GREEN`、`RED`、`YELLOW`，再通过 USB 串口、局域网 UDP 或两者同时发送给 ESP32-C3。固件驱动三颗独立 WS2812B LED 显示空闲、工作和等待状态。

本项目是社区开源项目，与 OpenAI 没有官方关联或背书。

## 当前可用方案

- 配网使用 USB 串口，不再使用 ESP32 AP 热点页面。
- 托盘程序提供 `Configure WiFi`，通过 USB 把 SSID 和密码发送给设备。
- Wi-Fi 只在连接成功后写入 ESP32 NVS；错误密码不会覆盖旧配置。
- 已保存的 Wi-Fi 会在开机后非阻塞连接，并在断线后持续重试。
- ESP32-C3 Wi-Fi 发射功率默认限制为 8.5 dBm，避免部分 Super Mini 板认证超时。
- 无电脑供电启动不会依赖 USB Serial；固件已移除启动阶段可能阻塞的 `Serial.flush()`。
- 支持 `AUTO`、`WIRED`、`WIRELESS` 三种持久化通信模式。
- `Bridge/start_codex_light_tray.vbs` 默认以无线模式隐藏启动，不保留 PowerShell 窗口。

## 状态定义

| 状态 | Codex 条件 | LED | GPIO |
| --- | --- | --- | --- |
| `GREEN` | 空闲、任务完成或任务中止 | 绿灯 | GPIO6 |
| `RED` | 正在思考、回复、运行工具或处理任务 | 红灯 | GPIO7 |
| `YELLOW` | 等待审批、权限确认或用户输入 | 黄灯 | GPIO5 |
| 未收到电脑心跳 | 6 秒内没有有效 USB/UDP 心跳 | 黄灯慢闪 | GPIO5 |

首次收到有效电脑心跳时，绿灯会闪烁 2 秒作为连接提示，然后显示真实状态。任务完成后的绿色会保持到下一次任务开始、进入等待状态或连接断开。

## 快速开始

1. 安装电脑端依赖：

   ```powershell
   python -m pip install pyserial
   ```

2. 编译并烧录固件：

   ```powershell
   cd Firmware
   pio run
   pio run -t upload --upload-port COM4
   ```

3. 双击启动隐藏托盘，默认进入无线模式：

   ```text
   Bridge\start_codex_light_tray.vbs
   ```

4. 第一次使用时，用 USB 连接 ESP32-C3，右键托盘图标选择 `Configure WiFi`，输入路由器 SSID 和密码。
5. Wi-Fi 保存成功后，可以拔掉电脑 USB，改用充电宝或锂电池供电；托盘会通过 UDP 控灯。

完整流程见 [USAGE.md](USAGE.md)。

## 纯无线使用

1. 先用 USB 完成一次 `Configure WiFi`。
2. 确认托盘模式是 `Wireless only`，或用默认 `start_codex_light_tray.vbs` 启动。
3. 断开电脑 USB，使用充电宝或稳定 5 V 电源给 ESP32-C3 和 LED 供电。
4. 打开托盘后，日志应出现：

   ```text
   SERIAL no matching serial port
   SERIAL setup skipped; using saved firmware mode.
   UDP ack from 192.168.x.x CODEXLIGHT/1 ACK ... active=WIRELESS state=...
   ```

这表示设备未接电脑串口，但已通过 Wi-Fi UDP 正常工作。

## 无串口诊断灯码

| 灯码 | 含义 |
| --- | --- |
| 红黄交替 | 没有读到 Wi-Fi 配置，需要 USB 配网 |
| 红色双闪循环 | 有 Wi-Fi 配置，但正在重连或连接失败 |
| 黄灯慢闪 | Wi-Fi 已连接，正在等待电脑托盘 UDP 心跳 |
| 正常红/绿/黄 | 已收到电脑状态，系统正常工作 |

## 硬件

推荐器件：

- ESP32-C3 SuperMini 或兼容 ESP32-C3 开发板
- 三颗 WS2812B RGB LED
- 稳定 5 V 电源、充电宝或合适的锂电池升压模块
- USB 数据线，用于烧录和第一次 Wi-Fi 配网

接线：

| 功能 | ESP32-C3 引脚 |
| --- | --- |
| 黄灯 DIN | GPIO5 |
| 绿灯 DIN | GPIO6 |
| 红灯 DIN | GPIO7 |
| WS2812B VCC | 5 V |
| WS2812B GND | GND |

三颗 WS2812B 使用三路独立数据输入，不是串联灯带。当前固件使用 `NEO_GRB + NEO_KHZ800`，全局亮度在 [Firmware/include/config.h](Firmware/include/config.h) 的 `DEFAULT_BRIGHTNESS` 中配置。

## 目录结构

```text
CodexLight/
├── Bridge/        # Windows 日志监听、串口/UDP 发送和托盘程序
├── Firmware/      # ESP32-C3 PlatformIO 固件
├── Hardware/      # BOM、原理图、PCB、Gerber、外壳 STL
├── Docs/          # 使用与实现说明
├── README.md      # 中文项目说明
├── README.en.md   # English project documentation
├── USAGE.md       # 中文使用手册
└── USAGE.en.md    # English usage guide
```

## 本地文件和隐私

不要提交以下本机运行状态：

- `Bridge/config.local.json`：本机发现的设备 MAC/IP。
- `Bridge/logs/`：运行日志和 Wi-Fi 配网日志。
- `.pio/`、`Firmware/.pio/`：PlatformIO 构建产物。
- Wi-Fi 密码、私有 SSID、本机端口号和临时诊断文件。

## 验证

```powershell
python -B -m py_compile Bridge\codex_light_monitor.py
powershell -NoProfile -Command "$e=$null; [System.Management.Automation.PSParser]::Tokenize((Get-Content -LiteralPath 'Bridge\CodexLightTray.ps1' -Raw), [ref]$e) | Out-Null; if($e){$e; exit 1}else{'OK'}"
cd Firmware
pio run
```

## 许可

本项目使用 [MIT License](LICENSE)。第三方依赖遵循各自许可。
