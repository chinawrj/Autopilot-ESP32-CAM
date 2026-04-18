# Autopilot-ESP32-CAM - Copilot Instructions

## Project Overview

This is an autonomous driving development project based on the **VCC-GND Studio YD-ESP32-CAM** development board.
The AI Agent acts as a senior embedded engineer, iterating daily to autonomously deliver the full development lifecycle from scratch to completion.

## Hardware Platform

- **Dev Board**: YD-ESP32-CAM (VCC-GND Studio)
- **Core Module**: ESP32-WROVER-E-N8R8 (Flash 8MB, PSRAM 8MB)
- **Core Chip**: ESP32-D0WD-V3 (Dual-core Xtensa LX6, 240MHz)
- **Camera**: OV2640
- **Onboard LED**: GPIO33

## Development Rules

### 0. Iron Rules (Highest Priority)

- **All terminal commands run via tmux** — Use sentinel mode from `.github/skills/tmux-multi-shell/SKILL.md`
- **Every code change must be tested on-device** — build → flash → monitor → verify, see `.github/skills/automated-testing/SKILL.md`
- **Compilation passing ≠ done** — Must observe expected behavior in serial logs
- **Browser must use visible mode (headless=False)** — No headless mode; use `~/.patchright-userdata` persistence directory
- **No restrictions on available tools** — Use every available tool

### 1. WiFi Credential Security

**Strictly forbidden** to hardcode WiFi passwords into source code or any Git-tracked file.

WiFi credentials are read from the following sources (by priority):
1. Environment variables: `ESP_WIFI_SSID` / `ESP_WIFI_PASSWORD`
2. Config file: `~/.esp-wifi-credentials` (format in .github/skills/wifi-credentials/)
3. ESP-IDF menuconfig: `Component config → WiFi Configuration`

### 2. Iterative Workflow

- Each conversation is treated as one "work day"
- Each work day has clear small goals (1–3 tasks)
- Each task is tested and verified immediately after completion
- Use `docs/daily-logs/day-NNN.md` to log daily work
- Use `docs/TARGET.md` to track overall progress

### 3. Git Commit Conventions

```
feat: New feature
fix: Bug fix
refactor: Refactoring
docs: Documentation
test: Testing
chore: Build/tooling
```

### 4. Build Environment

```bash
# Source the ESP-IDF environment first
. $HOME/esp/esp-idf/export.sh
# Or use the idf.py shortcut (if PATH is configured)
```

### 5. Serial Port

The YD-ESP32-CAM has no onboard USB-to-serial converter; an external USB-TTL adapter is required:
- TX → GPIO1 (U0TXD)
- RX → GPIO3 (U0RXD)
- GND → GND
- Baud rate: 115200

### 6. Camera Pins (AI-Thinker Compatible)

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
