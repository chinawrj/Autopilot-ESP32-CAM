# Skill: 开发板引脚与资源速查

## 用途

为 AI Agent 提供开发板的硬件引脚定义（Pinout）、核心模组参数、原理图要点等速查信息，
避免在开发过程中因引脚映射错误导致外设不工作或硬件损坏。

**何时使用：**
- 需要确认某个 GPIO 对应的物理引脚或功能
- 编写驱动代码前需要查看摄像头/SD 卡/串口等外设的引脚映射
- 需要了解开发板使用的核心模组型号、Flash/PSRAM 容量等参数
- 接线或设计外围电路时需要 pinout 参考
- 调试时需要确认某个引脚是否为 input-only 或有特殊限制

**何时不使用：**
- 芯片底层寄存器编程（应查阅乐鑫官方 TRM）
- 使用标准开发框架的 API 调用（如 `gpio_set_level`）
- 开发板不在本 Skill 的支持列表中

## 前置条件

- 无特殊依赖，本 Skill 为纯数据参考
- 如需查询不在列表中的开发板，可使用 `web-page-inspector` Skill 从厂商网站抓取

## 支持的开发板

| 厂商 | 型号 | 核心模组 | 状态 |
|------|------|---------|------|
| 源地工作室 (VCC-GND Studio) | YD-ESP32-CAM | ESP32-WROVER-E-N8R8 | ✅ 已收录 |

> 持续补充中。贡献新开发板请参考文末"添加新开发板"章节。

---

## 开发板: YD-ESP32-CAM（源地工作室）

### 概要

| 参数 | 值 |
|------|-----|
| **厂商** | 源地工作室 VCC-GND Studio (vcc-gnd.com) |
| **型号** | YD-ESP32-CAM |
| **核心芯片** | ESP32-D0WD-V3 (双核 Xtensa LX6, 240MHz) |
| **核心模组** | 乐鑫 ESP32-WROVER-E-N8R8 |
| **Flash** | 8 MB |
| **PSRAM** | 8 MB (外扩) |
| **摄像头** | OV2640（默认），兼容 OV3660 |
| **TF 卡槽** | 有 (Micro SD, SPI 模式) |
| **板载 LED** | GPIO33 |
| **BOOT 按键** | GPIO0 |
| **供电** | 5V (VIN) / 3.3V |
| **USB 转串口** | 无板载（需外接 USB-TTL 烧录） |

### ESP32 引脚复用 (PINMUX) 说明

> ⚠️ **理解 PINMUX 是正确使用引脚表的前提。**

ESP32 的外设引脚分配有两种机制，不同外设使用的机制不同：

| 机制 | 原理 | 可否重映射 | 本板涉及的外设 |
|------|------|-----------|--------------|
| **GPIO Matrix** | 信号通过可编程交换矩阵路由到 GPIO | ✅ 可任意映射 | 摄像头 (I2S)、UART、I2C (SCCB) |
| **IO MUX** | 信号直连到固定 GPIO，无矩阵延迟 | ❌ 硬件固定 | SDMMC Host Slot 2 (HS2) |

**对引脚表的影响：**

- **摄像头引脚**：通过 GPIO Matrix 路由。表中的 GPIO 编号是**板卡 PCB 设计者选定的**，
  不是 ESP32 芯片强制的。不同厂商的 ESP32-CAM 板可能使用不同映射。
  代码中需要用 `#define` 配置每个引脚。
- **TF 卡 (SDMMC) 引脚**：使用 I/O MUX 直连到 SDMMC HS2 Slot。GPIO 编号是
  **ESP32 芯片硬件固定的**，所有 ESP32 板卡都一样，无法更改。
  如果用 SPI 模式访问 SD 卡，则可通过 GPIO Matrix 重映射。
- **串口引脚**：UART0 默认 IO MUX 分配 GPIO1(TX)/GPIO3(RX)，但可通过 GPIO Matrix 重映射到其他引脚。
- **I2C (SCCB) 引脚**：通过 GPIO Matrix 路由，可任意映射。

> 参考: [ESP32 Technical Reference Manual - IO MUX and GPIO Matrix](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_cn.pdf) §4

### 摄像头接口引脚 (OV2640/OV3660)

> 🔀 **路由方式: GPIO Matrix** — 以下引脚由板卡 PCB 设计决定，非芯片固定

| 摄像头信号 | ESP32 GPIO | 说明 |
|-----------|-----------|------|
| D0 | GPIO5 | 数据线 bit0 |
| D1 | GPIO18 | 数据线 bit1 |
| D2 | GPIO19 | 数据线 bit2 |
| D3 | GPIO21 | 数据线 bit3 |
| D4 | GPIO36 | 数据线 bit4 (input-only) |
| D5 | GPIO39 | 数据线 bit5 (input-only) |
| D6 | GPIO34 | 数据线 bit6 (input-only) |
| D7 | GPIO35 | 数据线 bit7 (input-only) |
| XCLK | GPIO0 | 主时钟输出 (与 BOOT 按键共用) |
| PCLK | GPIO22 | 像素时钟输入 |
| VSYNC | GPIO25 | 帧同步 |
| HREF | GPIO23 | 行同步 |
| SDA | GPIO26 | SCCB 数据线 (I2C) |
| SCL | GPIO27 | SCCB 时钟线 (I2C) |
| POWER GPIO | GPIO32 | 摄像头电源控制 (低有效) |

### TF 卡 (Micro SD) 引脚

> 📌 **路由方式: IO MUX (SDMMC HS2)** — 以下引脚为 ESP32 芯片硬件固定，不可更改

| TF 卡信号 | ESP32 GPIO | SDMMC HS2 功能 | 说明 |
|----------|-----------|---------------|------|
| CLK | GPIO14 | HS2_CLK | SDMMC 时钟 |
| CMD | GPIO15 | HS2_CMD | SDMMC 命令线 |
| DATA0 | GPIO2 | HS2_DATA0 | SDMMC 数据线 0 |
| DATA1 | GPIO4 | HS2_DATA1 | 数据线 1 (1-bit 模式不用) |
| DATA2 | GPIO12 | HS2_DATA2 | 数据线 2 (⚠️ 启动时需低电平) |
| DATA3 | GPIO13 | HS2_DATA3 | 数据线 3 / SPI CS |

> ⚠️ **GPIO12** 是 MTDI 引脚，ESP32 启动时读取此引脚决定 VDD_SDIO 电压。
> 如果 SD 卡在上电时拉高 GPIO12，可能导致 Flash 供电电压错误而启动失败。
> 解决方案：使用 `espefuse.py set_flash_voltage 3.3V` 强制固定电压。

### 串口引脚

> 🔀 **路由方式: IO MUX 默认 + GPIO Matrix 可重映射**

| 信号 | ESP32 GPIO | IO MUX 功能 | 说明 |
|------|-----------|------------|------|
| TX | GPIO1 | U0TXD | UART0 默认发送 (IO MUX Function 1) |
| RX | GPIO3 | U0RXD | UART0 默认接收 (IO MUX Function 1) |

> 注: 源地原厂文档中串口 GPIO 栏为空，实际为 GPIO1/GPIO3 (UART0 默认 IO MUX 引脚)。
> UART 信号可通过 GPIO Matrix 重映射到其他引脚，但本板无板载 USB 转串口，
> 外接 USB-TTL 模块时需连接这两个默认引脚。

### 其他引脚

| 功能 | ESP32 GPIO | 说明 |
|------|-----------|------|
| BOOT 按键 | GPIO0 | 按住上电进入下载模式；与 XCLK 共用 |
| 板载 LED | GPIO33 | 高电平点亮 |
| Flash LED | GPIO4 | 闪光灯 (与 SD DAT1 共用) |

### GPIO 使用冲突注意事项

```
⚠️ 关键冲突:
├── GPIO0:  XCLK (摄像头) + BOOT 按键 → 烧录时需断开摄像头或按住 BOOT
├── GPIO4:  Flash LED + SD DAT1 → 使用 SD 4-bit 模式时闪光灯不可用
├── GPIO12: SD DAT2 + MTDI (启动电压选择) → 需要 efuse 固定电压
├── GPIO2:  SD DAT0 → 某些烧录器要求 GPIO2 悬空才能进入下载模式
└── GPIO34-39: 仅输入 (input-only)，不能用作输出
```

### ESP-IDF 摄像头配置参考

```c
// camera_pins.h for YD-ESP32-CAM
#define CAMERA_MODEL_YD_ESP32_CAM

#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1  // 无硬件复位引脚
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26  // SDA
#define SIOC_GPIO_NUM    27  // SCL

#define Y9_GPIO_NUM      35  // D7
#define Y8_GPIO_NUM      34  // D6
#define Y7_GPIO_NUM      39  // D5
#define Y6_GPIO_NUM      36  // D4
#define Y5_GPIO_NUM      21  // D3
#define Y4_GPIO_NUM      19  // D2
#define Y3_GPIO_NUM      18  // D1
#define Y2_GPIO_NUM       5  // D0

#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22
```

### Arduino 摄像头配置参考

```cpp
// For YD-ESP32-CAM (identical to AI-Thinker ESP32-CAM pinout)
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
```

> 注: YD-ESP32-CAM 的摄像头引脚与 AI-Thinker ESP32-CAM 完全兼容，
> Arduino 库中可直接选择 `CAMERA_MODEL_AI_THINKER`。

---

## 核心模组参考: ESP32-WROVER-E-N8R8

| 参数 | 值 |
|------|-----|
| **芯片** | ESP32-D0WD-V3 |
| **CPU** | 双核 Xtensa LX6, 最高 240 MHz |
| **SRAM** | 520 KB |
| **Flash** | 8 MB (Quad SPI) |
| **PSRAM** | 8 MB (Octal SPI) |
| **WiFi** | 802.11 b/g/n, 2.4 GHz |
| **Bluetooth** | BT 4.2 + BLE |
| **工作温度** | -40°C ~ 85°C |
| **供电电压** | 3.0V ~ 3.6V |
| **天线** | PCB 板载天线 |
| **封装尺寸** | 18mm × 20mm × 3.2mm |
| **认证** | FCC / CE / TELEC / KCC |

> 乐鑫官方数据手册: https://www.espressif.com/sites/default/files/documentation/esp32-wrover-e_esp32-wrover-ie_datasheet_cn.pdf

---

## 添加新开发板

向本 Skill 贡献新开发板信息时，请遵循以下模板：

```markdown
## 开发板: <型号>（<厂商>）

### 概要
| 参数 | 值 |
|------|-----|
| **厂商** | ... |
| **型号** | ... |
| **核心模组** | ... |
| **Flash** | ... |
| **PSRAM** | ... |
| ... | ... |

### <外设名> 引脚
| 信号 | GPIO | 说明 |
|------|------|------|
| ... | ... | ... |

### GPIO 使用冲突注意事项
列出已知的引脚冲突和启动限制。

### 代码配置参考
给出 ESP-IDF / Arduino / MicroPython 的引脚配置代码片段。
```

**数据来源要求：**
- 厂商官方文档或原理图（优先）
- 实物丝印标注
- 社区验证的第三方资料（需注明来源）

---

## Self-Test（自检）

### 自检步骤

验证 SKILL.md 中的引脚数据完整性和一致性。

```bash
bash skills/board-pinout-reference/self-test.sh
```

### 预期结果

| 测试项 | 预期 |
|--------|------|
| SKILL.md 存在 | PASS |
| 至少收录 1 块开发板 | PASS |
| 摄像头引脚数量 = 15 | PASS |
| TF 卡引脚数量 = 6 | PASS |
| 串口引脚数量 = 2 | PASS |
| GPIO 编号范围合法 (0-39) | PASS |
| 无重复 GPIO 映射（同一外设内） | PASS |
| ESP-IDF 配置代码与引脚表一致 | PASS |
| 包含冲突注意事项 | PASS |
| 包含模组参考章节 | PASS |

### Blind Test（盲测）

**场景描述:**
一个新的 AI Agent 需要为 YD-ESP32-CAM 开发板编写摄像头初始化代码，并正确配置 SD 卡。

**测试 Prompt:**
> 我有一块源地工作室的 YD-ESP32-CAM 开发板，请帮我：
> 1. 给出摄像头 OV2640 的引脚配置（ESP-IDF 风格的 #define）
> 2. 告诉我 SD 卡使用了哪些 GPIO，有什么注意事项
> 3. 板载 LED 是哪个 GPIO

**验收标准:**
- [ ] 摄像头配置包含全部 15 个引脚定义 (PWDN, RESET, XCLK, SDA, SCL, D0-D7, VSYNC, HREF, PCLK)
- [ ] SD 卡列出 6 个引脚 (CLK=14, CMD=15, DATA0=2, DATA1=4, DATA2=12, DATA3=13)
- [ ] 明确提到 GPIO12 启动电压冲突及 `espefuse.py` 解决方案
- [ ] 板载 LED 正确标注为 GPIO33
- [ ] 提到 GPIO0 的 XCLK/BOOT 双重功能冲突

**常见失败模式:**
- Agent 混淆 AI-Thinker ESP32-CAM 与 YD-ESP32-CAM 的差异 → 实际引脚兼容，但模组不同
- Agent 给出错误的 PWDN 引脚（某些板子是 -1，YD 板是 GPIO32）
- Agent 遗漏 GPIO12 启动限制 → 导致用户板子无法启动

## 成功标准

- [ ] 引脚表数据来源可靠（厂商文档/实物验证）
- [ ] 每个外设的 GPIO 映射完整无遗漏
- [ ] 关键冲突和限制有明显标注（⚠️ 符号）
- [ ] 提供可直接复制到代码中的配置片段
- [ ] Self-Test 全部 PASS
