# CodexLight

[English](README.en.md) | 简体中文 | [中文使用说明](USAGE.md) | [English Usage Guide](USAGE.en.md)

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
├─ USAGE.md                # 中文使用说明
├─ USAGE.en.md             # English usage guide
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

## 使用说明

完整操作流程已经独立到根目录使用手册：

- [中文使用说明](USAGE.md)
- [English Usage Guide](USAGE.en.md)

使用手册包含固件编译与烧录、手机 AP 配网、有线/无线/AUTO 模式、Windows 托盘切换、串口和 UDP 协议、状态规则、故障排查、安全说明与开发验证。

快速启动托盘程序：

```text
Bridge\start_codex_light_tray.bat
```

## 文档

- [Bridge 中文说明](Bridge/README.md)
- [Bridge English Guide](Bridge/README_en.md)
- [中文使用说明](USAGE.md)
- [English Usage Guide](USAGE.en.md)
- [使用与实现说明](Docs/使用与实现说明.md)
- [Usage and Implementation Guide](Docs/USAGE_AND_IMPLEMENTATION.md)

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
