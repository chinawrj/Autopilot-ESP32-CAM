#!/bin/bash
# self-test for board-pinout
# 运行: bash skills/board-pinout/self-test.sh

SKILL="skills/board-pinout/SKILL.md"
PASS=0; FAIL=0

tp() { echo "SELF_TEST_PASS: $1"; PASS=$((PASS+1)); }
tf() { echo "SELF_TEST_FAIL: $1"; FAIL=$((FAIL+1)); }

[ -f "$SKILL" ] && tp "skill_md_exists" || tf "skill_md_exists"
grep -q "PWDN.*32" "$SKILL" && tp "cam_pwdn_gpio32" || tf "cam_pwdn_gpio32"
grep -q "XCLK.*0" "$SKILL" && tp "cam_xclk_gpio0" || tf "cam_xclk_gpio0"
grep -q "SDA.*26" "$SKILL" && tp "cam_sda_gpio26" || tf "cam_sda_gpio26"
grep -q "CLK.*14" "$SKILL" && tp "sd_clk_gpio14" || tf "sd_clk_gpio14"
grep -q "GPIO0.*BOOT" "$SKILL" && tp "conflict_gpio0_boot" || tf "conflict_gpio0_boot"
grep -q "GPIO12.*espefuse" "$SKILL" && tp "conflict_gpio12" || tf "conflict_gpio12"
grep -q "GPIO33" "$SKILL" && tp "led_gpio33" || tf "led_gpio33"

echo ""; echo "Results: $PASS passed, $FAIL failed"
exit $FAIL
