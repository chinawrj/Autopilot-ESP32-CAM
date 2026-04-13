#!/bin/bash
# self-test for tmux-session
# 运行: bash skills/tmux-session/self-test.sh

SKILL="skills/tmux-session/SKILL.md"
PASS=0; FAIL=0

tp() { echo "SELF_TEST_PASS: $1"; PASS=$((PASS+1)); }
tf() { echo "SELF_TEST_FAIL: $1"; FAIL=$((FAIL+1)); }

[ -f "$SKILL" ] && tp "skill_md_exists" || tf "skill_md_exists"
command -v tmux &>/dev/null && tp "tmux_installed" || tf "tmux_installed"
grep -q "has-session" "$SKILL" && tp "idempotent_create" || tf "idempotent_create"
grep -q "sentinel\|__DONE_" "$SKILL" && tp "sentinel_pattern" || tf "sentinel_pattern"
grep -q "capture-pane" "$SKILL" && tp "capture_pane" || tf "capture_pane"
grep -q "tmux_exec" "$SKILL" && tp "tmux_exec_function" || tf "tmux_exec_function"
grep -q "C-c\|C-]" "$SKILL" && tp "kill_recovery" || tf "kill_recovery"

echo ""; echo "Results: $PASS passed, $FAIL failed"
exit $FAIL
