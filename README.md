# CodexLight

简体中文 | [English](README.en.md)

CodexLight 是一个基于 ESP32-C3 SuperMini 的三色状态灯项目。当前阶段已完成固件基础框架：使用 PlatformIO、Arduino Framework 和 FastLED 驱动 3 颗独立 WS2812B，用于 Codex 状态提示的硬件显示。

## Current Status

- 已建立 PlatformIO 固件工程，工程目录位于 `Firmware/`。
- 已接入 FastLED 依赖。
- 已封装 `LedController`，`main.cpp` 不直接调用 FastLED API。
- 已支持 3 颗独立 WS2812B 单灯珠控制。
- 已将 GPIO、默认亮度、颜色参数集中放在 `config.h`。
- 已通过 `pio run` 编译验证。
- 已加入硬件原理图资料，位于 `Hardware/Schematic/`。
- 已预留 `Bridge/` 和 `Docs/` 目录，用于后续桥接程序和项目文档。

## Hardware Configuration

主控：ESP32-C3 SuperMini

LED：3 颗独立 WS2812B，不是串联灯带。每颗 LED 都有独立数据线，每路 `NUM_LEDS = 1`。

| LED | GPIO | 显示颜色 |
| --- | --- | --- |
| Red | GPIO7 | 红色 |
| Green | GPIO6 | 绿色 |
| Yellow | GPIO5 | 黄色 |

硬件连接要求：

- 每个 WS2812B 的 DIN 串联 330Ω 电阻。
- 每颗 WS2812B 电源旁放置 100nF 去耦电容。
- LED 电源地与 ESP32-C3 地线共地。

## Project Structure

```text
CodexLight/
├── README.md
├── README.en.md
├── .gitignore
├── Bridge/
│   └── .gitkeep
├── Docs/
│   └── .gitkeep
├── Firmware/
│   ├── platformio.ini
│   ├── include/
│   │   ├── config.h
│   │   └── led.h
│   ├── src/
│   │   ├── main.cpp
│   │   └── led.cpp
│   ├── lib/
│   └── test/
└── Hardware/
    └── Schematic/
        └── Schematic1.pdf
```

目录说明：

- `Bridge/`：桥接程序目录，预留给电脑端、插件或状态同步程序。
- `Docs/`：文档目录，预留给项目说明、开发记录和使用文档。
- `Firmware/`：固件目录，存放 ESP32-C3 的 PlatformIO 工程。
- `Hardware/`：硬件目录，存放原理图和后续硬件资料。

## Firmware Framework

固件工程位于 `Firmware/`，当前环境配置如下：

```ini
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
lib_deps = fastled/FastLED
```

### Configuration

`Firmware/include/config.h` 负责集中维护硬件和显示参数：

- `RED_LED_PIN = 7`
- `GREEN_LED_PIN = 6`
- `YELLOW_LED_PIN = 5`
- `LEDS_PER_CHANNEL = 1`
- `DEFAULT_BRIGHTNESS = 64`
- 红、绿、黄三种显示颜色的 RGB 参数

### LED Controller

`Firmware/include/led.h` 定义 `LedController` 对外接口：

```cpp
void begin();

void redOn();
void redOff();

void greenOn();
void greenOff();

void yellowOn();
void yellowOff();

void allOn();
void allOff();

void setBrightness(uint8_t brightness);
```

`Firmware/src/led.cpp` 内部使用 FastLED，并为每个 GPIO 创建独立控制器：

```cpp
FastLED.addLeds<WS2812B, RED_LED_PIN, GRB>(redLed, LEDS_PER_CHANNEL);
FastLED.addLeds<WS2812B, GREEN_LED_PIN, GRB>(greenLed, LEDS_PER_CHANNEL);
FastLED.addLeds<WS2812B, YELLOW_LED_PIN, GRB>(yellowLed, LEDS_PER_CHANNEL);
```

`Firmware/src/main.cpp` 只负责初始化和调用：

```cpp
LedController leds;

void setup() {
  leds.begin();
  leds.allOn();
}

void loop() {
}
```

当前上电效果：红、绿、黄三颗 LED 全部点亮。

## Hardware Files

硬件资料位于 `Hardware/`：

- `Hardware/Schematic/Schematic1.pdf`：当前原理图文件。

## License

MIT
