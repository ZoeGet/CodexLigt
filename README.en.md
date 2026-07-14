# CodexLight

[简体中文](README.md) | English

CodexLight is a three-color status light project based on ESP32-C3 SuperMini. The current firmware foundation is implemented with PlatformIO, Arduino Framework, and FastLED. It drives three independent WS2812B LEDs for hardware display of Codex status signals.

## Current Status

- PlatformIO firmware project has been created under `Firmware/`.
- FastLED dependency has been added.
- `LedController` has been implemented, and `main.cpp` does not call the FastLED API directly.
- Three independent single-pixel WS2812B LEDs are supported.
- GPIO pins, default brightness, and color parameters are centralized in `config.h`.
- Firmware has been verified with `pio run`.
- Hardware schematic files have been added under `Hardware/Schematic/`.
- `Bridge/` and `Docs/` are reserved for the future bridge program and project documentation.

## Hardware Configuration

MCU: ESP32-C3 SuperMini

LEDs: three independent WS2812B LEDs. They are not connected as a chained strip. Each LED has its own data line, and each channel uses `NUM_LEDS = 1`.

| LED | GPIO | Color |
| --- | --- | --- |
| Red | GPIO7 | Red |
| Green | GPIO6 | Green |
| Yellow | GPIO5 | Yellow |

Hardware connection notes:

- Each WS2812B DIN line has a 330Ω series resistor.
- Each WS2812B has a 100nF decoupling capacitor near its power pins.
- LED power ground and ESP32-C3 ground must be connected together.

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

Directory overview:

- `Bridge/`: reserved for the desktop-side, plugin, or status bridge program.
- `Docs/`: reserved for project documentation.
- `Firmware/`: ESP32-C3 firmware project.
- `Hardware/`: schematic and future hardware files.

## Firmware Framework

The firmware project is located in `Firmware/`. Current PlatformIO environment:

```ini
[env:esp32-c3-devkitm-1]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
lib_deps = fastled/FastLED
```

### Configuration

`Firmware/include/config.h` centralizes hardware and display parameters:

- `RED_LED_PIN = 7`
- `GREEN_LED_PIN = 6`
- `YELLOW_LED_PIN = 5`
- `LEDS_PER_CHANNEL = 1`
- `DEFAULT_BRIGHTNESS = 64`
- RGB parameters for red, green, and yellow display colors

### LED Controller

`Firmware/include/led.h` defines the public `LedController` API:

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

`Firmware/src/led.cpp` uses FastLED internally and creates one independent controller for each GPIO:

```cpp
FastLED.addLeds<WS2812B, RED_LED_PIN, GRB>(redLed, LEDS_PER_CHANNEL);
FastLED.addLeds<WS2812B, GREEN_LED_PIN, GRB>(greenLed, LEDS_PER_CHANNEL);
FastLED.addLeds<WS2812B, YELLOW_LED_PIN, GRB>(yellowLed, LEDS_PER_CHANNEL);
```

`Firmware/src/main.cpp` only handles initialization and calls the LED controller:

```cpp
LedController leds;

void setup() {
  leds.begin();
  leds.allOn();
}

void loop() {
}
```

Current power-on behavior: the red, green, and yellow LEDs all turn on.

## Hardware Files

Hardware files are located under `Hardware/`:

- `Hardware/Schematic/Schematic1.pdf`: current schematic file.

## License

MIT
