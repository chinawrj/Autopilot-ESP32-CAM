#!/bin/bash
# self-test for daily-iteration
# 运行: bash skills/daily-iteration/self-test.sh

SKILL="skills/daily-iteration/SKILL.md"
PASS=0; FAIL=0

tp() { echo "SELF_TEST_PASS: $1"; PASS=$((PASS+1)); }
tf() { echo "SELF_TEST_FAIL: $1"; FAIL=$((FAIL+1)); }

[ -f "$SKILL" ] && tp "skill_md_exists" || tf "skill_md_exists"
grep -q "Morning" "$SKILL" && tp "has_morning_section" || tf "has_morning_section"
grep -q "Evening" "$SKILL" && tp "has_evening_section" || tf "has_evening_section"
grep -q "idf.py build" "$SKILL" && tp "has_build_step" || tf "has_build_step"
grep -q "git commit" "$SKILL" && tp "has_commit_step" || tf "has_commit_step"
grep -q "重构" "$SKILL" && tp "has_refactor_trigger" || tf "has_refactor_trigger"
[ -d "docs/daily-logs" ] && tp "daily_logs_dir" || tf "daily_logs_dir"

echo ""; echo "Results: $PASS passed, $FAIL failed"
exit $FAIL
