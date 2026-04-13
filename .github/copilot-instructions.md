# Autopilot-ESP32-CAM - Copilot Instructions

## 项目概述

这是一个基于 **源地工作室 YD-ESP32-CAM** 开发板的自动驾驶开发项目。
AI Agent 将扮演资深嵌入式工程师，通过每日迭代方式自动完成从零到交付的全流程开发。

## 硬件平台

- **开发板**: YD-ESP32-CAM (VCC-GND Studio)
- **核心模组**: ESP32-WROVER-E-N8R8 (Flash 8MB, PSRAM 8MB)
- **核心芯片**: ESP32-D0WD-V3 (双核 Xtensa LX6, 240MHz)
- **摄像头**: OV2640
- **板载 LED**: GPIO33

## 开发规则

### 0. 铁律（最高优先级）

- **所有终端命令通过 tmux 执行** — 使用 `skills/tmux-multi-shell/SKILL.md` 中的 sentinel 模式
- **每次代码变更必须上板测试** — build → flash → monitor → 验证，参见 `skills/automated-testing/SKILL.md`
- **编译通过 ≠ 完成** — 必须看到串口日志中的预期行为
- **浏览器必须可见模式 (headless=False)** — 禁止 headless，使用 `~/.patchright-userdata` 持久化目录
- **不限制可用工具** — 使用一切可用的 tool

### 1. WiFi 凭据安全

**绝对禁止** 将 WiFi 密码硬编码到源代码或任何被 Git 跟踪的文件中。

WiFi 凭据从以下位置读取（按优先级）：
1. 环境变量: `ESP_WIFI_SSID` / `ESP_WIFI_PASSWORD`
2. 配置文件: `~/.esp-wifi-credentials`（格式见 skills/wifi-credentials/）
3. ESP-IDF menuconfig: `Component config → WiFi Configuration`

### 2. 迭代工作流

- 每次对话视为一个"工作日"
- 每个工作日有明确的小目标（1-3个任务）
- 每个任务完成后立即测试验证
- 使用 `docs/daily-logs/day-NNN.md` 记录每日工作
- 使用 `docs/TARGET.md` 跟踪总体进度

### 3. Git 提交规范

```
feat: 新功能
fix: Bug 修复
refactor: 重构
docs: 文档
test: 测试
chore: 构建/工具
```

### 4. 编译环境

```bash
# 需要先 source ESP-IDF 环境
. $HOME/esp/esp-idf/export.sh
# 或者使用 idf.py 快捷方式（如果已配置 PATH）
```

### 5. 串口

YD-ESP32-CAM 无板载 USB 转串口，需要外接 USB-TTL：
- TX → GPIO1 (U0TXD)
- RX → GPIO3 (U0RXD)
- GND → GND
- 波特率: 115200

### 6. 摄像头引脚 (AI-Thinker 兼容)

```c
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22
```
