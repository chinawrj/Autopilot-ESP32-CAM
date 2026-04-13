# Skill: YD-ESP32-CAM 引脚速查

## 用途

查找 YD-ESP32-CAM 开发板的 GPIO 引脚分配和硬件约束。

**何时使用：**
- 编写摄像头、SD 卡、LED 等外设初始化代码
- 排查 GPIO 冲突（启动失败、外设不工作）
- 需要确认某个 GPIO 能否用作输出

**何时不使用：**
- 与引脚无关的纯软件逻辑问题
- 不同型号开发板（AI-Thinker ESP32-CAM 等）

## 前置条件

无特殊依赖，仅需阅读本文档。

## 操作步骤

### 硬件概要

| 参数 | 值 |
|------|-----|
| 开发板 | YD-ESP32-CAM (VCC-GND Studio) |
| 模组 | ESP32-WROVER-E-N8R8 (8MB Flash + 8MB PSRAM) |
| 芯片 | ESP32-D0WD-V3 (双核 240MHz) |
| 摄像头 | OV2640 |
| LED | GPIO33 |
| BOOT | GPIO0 |

### 摄像头引脚 (GPIO Matrix — 板卡设计决定)

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

### SD 卡引脚 (IO MUX 固定 — 不可更改)

| 信号 | GPIO | 信号 | GPIO |
|------|------|------|------|
| CLK | 14 | DATA0 | 2 |
| CMD | 15 | DATA1 | 4 |
| DATA2 | 12 ⚠️ | DATA3 | 13 |

### 关键冲突

- **GPIO0**: XCLK + BOOT → 烧录时需断开摄像头
- **GPIO4**: Flash LED + SD DAT1 → SD 4-bit 模式时闪光灯不可用
- **GPIO12**: SD DAT2 + MTDI → 需 `espefuse.py set_flash_voltage 3.3V`
- **GPIO34-39**: 仅输入，不能做输出

## Self-Test（自检）

```bash
#!/bin/bash
SKILL="skills/board-pinout/SKILL.md"

[ -f "$SKILL" ] && echo "SELF_TEST_PASS: skill_md_exists" || echo "SELF_TEST_FAIL: skill_md_exists"
grep -q "PWDN.*32" "$SKILL" && echo "SELF_TEST_PASS: cam_pwdn_gpio32" || echo "SELF_TEST_FAIL: cam_pwdn_gpio32"
grep -q "XCLK.*0" "$SKILL" && echo "SELF_TEST_PASS: cam_xclk_gpio0" || echo "SELF_TEST_FAIL: cam_xclk_gpio0"
grep -q "SDA.*26" "$SKILL" && echo "SELF_TEST_PASS: cam_sda_gpio26" || echo "SELF_TEST_FAIL: cam_sda_gpio26"
grep -q "CLK.*14" "$SKILL" && echo "SELF_TEST_PASS: sd_clk_gpio14" || echo "SELF_TEST_FAIL: sd_clk_gpio14"
grep -q "GPIO0.*BOOT" "$SKILL" && echo "SELF_TEST_PASS: conflict_gpio0_boot" || echo "SELF_TEST_FAIL: conflict_gpio0_boot"
grep -q "GPIO12.*espefuse" "$SKILL" && echo "SELF_TEST_PASS: conflict_gpio12" || echo "SELF_TEST_FAIL: conflict_gpio12"
grep -q "GPIO33" "$SKILL" && echo "SELF_TEST_PASS: led_gpio33" || echo "SELF_TEST_FAIL: led_gpio33"
```

### Blind Test（盲测）

**场景描述:**
AI Agent 需要为 YD-ESP32-CAM 编写摄像头初始化代码。

**测试 Prompt:**
> 我要初始化 YD-ESP32-CAM 的 OV2640 摄像头，请给出所有引脚的 #define 配置。另外 SD 卡的引脚是什么？GPIO12 有什么注意事项？

**验收标准:**
- [ ] 摄像头配置包含 PWDN=32, XCLK=0, SDA=26, SCL=27, D0=5, D7=35
- [ ] SD 卡列出 CLK=14, CMD=15, DATA0=2
- [ ] 提到 GPIO12 需要 `espefuse.py set_flash_voltage 3.3V`
- [ ] 提到 GPIO0 XCLK/BOOT 冲突

**常见失败模式:**
- 混淆 AI-Thinker 与 YD 板的引脚差异
- 遗漏 GPIO12 启动电压限制

## 成功标准

- [ ] 摄像头 15 个引脚全部正确
- [ ] SD 卡 6 个引脚全部正确
- [ ] GPIO 冲突全部标注
- [ ] input-only GPIO (34-39) 已标注
