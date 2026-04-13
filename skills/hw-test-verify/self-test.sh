#!/bin/bash
# self-test for hw-test-verify
# 运行: bash skills/hw-test-verify/self-test.sh

SKILL="skills/hw-test-verify/SKILL.md"
PASS=0; FAIL=0

tp() { echo "SELF_TEST_PASS: $1"; PASS=$((PASS+1)); }
tf() { echo "SELF_TEST_FAIL: $1"; FAIL=$((FAIL+1)); }

[ -f "$SKILL" ] && tp "skill_md_exists" || tf "skill_md_exists"
grep -q "idf.py build" "$SKILL" && tp "has_build_step" || tf "has_build_step"
grep -q "idf.py flash" "$SKILL" && tp "has_flash_step" || tf "has_flash_step"
grep -q "idf.py monitor" "$SKILL" && tp "has_monitor_step" || tf "has_monitor_step"
grep -q "curl" "$SKILL" && tp "has_curl_verify" || tf "has_curl_verify"
grep -q "patchright" "$SKILL" && tp "has_browser_verify" || tf "has_browser_verify"
grep -q "headless=False" "$SKILL" && tp "headless_false" || tf "headless_false"
grep -q "tmux" "$SKILL" && tp "uses_tmux" || tf "uses_tmux"
grep -q "CH340\|wchusbserial" "$SKILL" && tp "ch340_serial" || tf "ch340_serial"

echo ""; echo "Results: $PASS passed, $FAIL failed"
exit $FAIL
