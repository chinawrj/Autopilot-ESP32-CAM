# Skill: YD-ESP32-CAM 引脚速查

## 硬件概要

| 参数 | 值 |
|------|-----|
| 开发板 | YD-ESP32-CAM (VCC-GND Studio) |
| 模组 | ESP32-WROVER-E-N8R8 (8MB Flash + 8MB PSRAM) |
| 芯片 | ESP32-D0WD-V3 (双核 240MHz) |
| 摄像头 | OV2640 |
| LED | GPIO33 |
| BOOT | GPIO0 |

## 摄像头引脚 (GPIO Matrix — 板卡设计决定)

| 信号 | GPIO | 信号 | GPIO |
|------|------|------|------|
| D0 | 5 | D4 | 36 (input-only) |
| D1 | 18 | D5 | 39 (input-only) |
| D2 | 19 | D6 | 34 (input-only) |
| D3 | 21 | D7 | 35 (input-only) |
| XCLK | 0 | PCLK | 22 |
| VSYNC | 25 | HREF | 23 |
| SDA | 26 | SCL | 27 |
| PWDN | 32 | RESET | -1 (无) |

## SD 卡引脚 (IO MUX 固定 — 不可更改)

| 信号 | GPIO | 信号 | GPIO |
|------|------|------|------|
| CLK | 14 | DATA0 | 2 |
| CMD | 15 | DATA1 | 4 |
| DATA2 | 12 ⚠️ | DATA3 | 13 |

## 关键冲突

- **GPIO0**: XCLK + BOOT → 烧录时需断开摄像头
- **GPIO4**: Flash LED + SD DAT1 → SD 4-bit 模式时闪光灯不可用
- **GPIO12**: SD DAT2 + MTDI → 需 `espefuse.py set_flash_voltage 3.3V`
- **GPIO34-39**: 仅输入，不能做输出
