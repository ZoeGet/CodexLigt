# CodexLight Bridge

简体中文 | [English](README_en.md)

`codex_light_monitor.py` 是 CodexLight 的电脑端监听脚本。它监听本机 Codex Desktop 日志，把 Codex 的活动状态映射为三种灯光状态：

- `GREEN`：空闲、完成，或一段时间没有新的 agent 活动
- `RED`：正在思考、编写、调用工具或执行动作
- `YELLOW`：正在等待批准，或等待明确的用户输入

## 日志来源

主要来源：

```text
C:\Users\<you>\.codex\sessions\YYYY\MM\DD\rollout-*.jsonl
```

脚本会监听 JSONL 文件的追加内容，例如 `task_started`、`function_call`、`function_call_output`、`agent_message`、`turn_aborted` 等事件。

辅助来源：

```text
C:\Users\<you>\.codex\logs_2.sqlite
```

脚本只把 SQLite 里的 app-server 诊断事件作为“完成提示”，例如 `app-server event: item/completed`。它不会把 SQLite 里的普通日志文本当作稳定公开 API。

## 状态规则

| 状态 | 规则 |
| --- | --- |
| `GREEN` | 启动默认状态、`turn_aborted`、app-server 完成提示，或静默超时 |
| `RED` | `task_started`、agent message/reasoning、工具调用、工具输出，或 SQLite `ERROR` |
| `YELLOW` | 工具调用 payload 中包含审批/用户输入标记，例如 `require_escalated`、`sandbox_permissions`、`request_user_input`、`approval`、`permission` |

当前检查到的 Codex Desktop 本地 JSONL 日志里，没有确认稳定的 `task_finished` 或 `waiting_for_user` 单一事件。因此脚本会用 app-server 完成提示和静默超时组合判断何时回到 `GREEN`。

## 运行

只在控制台测试：

```powershell
python Bridge\codex_light_monitor.py
```

从最近已有日志开头处理，适合调试状态规则：

```powershell
python Bridge\codex_light_monitor.py --from-start --quiet-timeout 5
```

通过串口把状态发送给 ESP32：

```powershell
python Bridge\codex_light_monitor.py --serial COM5 --baud 115200
```

## Win10 托盘后台运行

直接双击：

```text
Bridge\start_codex_light_tray.bat
```

它会用隐藏窗口启动 PowerShell 托盘包装器 `CodexLightTray.ps1`，然后在后台运行 `codex_light_monitor.py`。运行后可以在 Win10 右下角任务栏折叠区看到托盘图标。右键图标可以：

- 打开日志目录
- 重启监听脚本
- 退出后台程序

日志输出保存在：

```text
Bridge\logs\codex_light_monitor.out.log
Bridge\logs\codex_light_monitor.err.log
```

默认启动脚本会同时启用有线串口和无线 UDP：

```bat
set "MONITOR_ARGS=--serial auto --baud 115200 --udp --udp-port 4210"
```

如果只想使用有线串口：

```bat
set "MONITOR_ARGS=--serial auto --baud 115200"
```

如果只想使用无线 UDP：

```bat
set "MONITOR_ARGS=--udp --udp-port 4210"
```

自动模式会优先选择 ESP32、Espressif USB/JTAG、CP210x、CH340/CH341、FTDI、USB Serial、CDC/UART 等常见串口设备。串口断开后，脚本会周期性尝试重连。

如果自动模式选错设备，可以把 `auto` 改成固定串口，例如：

```bat
set "MONITOR_ARGS=--serial COM5 --baud 115200"
```

控制台模式也可以直接使用自动串口：

```powershell
python Bridge\codex_light_monitor.py --serial auto --baud 115200
```

控制台模式也可以使用 UDP 广播：

```powershell
python Bridge\codex_light_monitor.py --udp --udp-port 4210
```

UDP 输出协议是一行 ASCII 文本，默认发到 `255.255.255.255:4210`，并且每 2 秒重复发送一次当前状态作为心跳：

```text
CODEXLIGHT/1 GREEN
CODEXLIGHT/1 RED
CODEXLIGHT/1 YELLOW
```

串口输出协议是一行一个 ASCII 状态，只在状态变化时发送：

```text
GREEN
RED
YELLOW
```

固件端可以从 `Serial` 读取整行，然后调用对应的 LED 控制方法。

## ESP32 Wi-Fi 配置

无线 UDP 模式需要在固件中配置 Wi-Fi。复制示例文件：

```text
Firmware\include\wifi_secrets.example.h
```

为：

```text
Firmware\include\wifi_secrets.h
```

然后填写：

```cpp
#define CODEXLIGHT_WIFI_SSID "你的WiFi名称"
#define CODEXLIGHT_WIFI_PASSWORD "你的WiFi密码"
```

`wifi_secrets.h` 已加入 `.gitignore`，不会提交到 GitHub。没有这个文件时，固件仍然可以编译并通过 USB 串口工作。

## 常用参数

```text
--poll 0.5              轮询间隔，单位秒
--quiet-timeout 20      没有新活动多久后回到 GREEN
--complete-grace 1.5    app-server item/completed 后等待多久回到 GREEN
--repeat                即使状态没有变化，也重复打印/发送
```

## 说明

串口模式和自动串口扫描需要在运行脚本的 Python 环境中安装 pyserial：

```powershell
pip install pyserial
```

默认的控制台监听模式不需要任何第三方依赖。

UDP 模式不需要第三方 Python 依赖。

托盘包装器使用 Win10 自带的 PowerShell/.NET `NotifyIcon`，不需要额外安装托盘库。
