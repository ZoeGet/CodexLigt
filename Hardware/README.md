# CodexLight Hardware

[项目主页](../README.md) | [Project Home](../README.en.md)

本目录包含 CodexLight 的原理图、PCB 制造资料和 3D 打印外壳。

This directory contains the CodexLight schematic, PCB manufacturing files, and 3D-printable enclosure.

## Files

| Path | Description |
| --- | --- |
| `BOM/BOM.xlsx` | 元器件物料清单 / Bill of materials |
| `Schematic/Schematic1.pdf` | 电路原理图 / Schematic PDF |
| `PCB/Source/CodexLight.epro2` | 嘉立创 EDA PCB 源工程 / PCB source project |
| `PCB/Gerber/CodexLight_PCB_Gerber.zip` | PCB Gerber 制造包 / Gerber fabrication package |
| `Enclosure/CodexLight_T.stl` | 上壳 / Top enclosure |
| `Enclosure/CodexLight_B.stl` | 下壳 / Bottom enclosure |

## LED Connections

| LED | GPIO |
| --- | --- |
| Yellow WS2812B DIN | GPIO5 |
| Green WS2812B DIN | GPIO6 |
| Red WS2812B DIN | GPIO7 |

三颗灯使用独立数据线。供电地必须和 ESP32-C3 共地，并使用稳定的 5 V 电源。

The three LEDs use independent data lines. The LED supply and ESP32-C3 must share ground, and a stable 5 V supply is required.

## License

Unless a third-party component states otherwise, the hardware files are distributed under the repository's [MIT License](../LICENSE).
