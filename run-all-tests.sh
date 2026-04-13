#!/bin/bash
# Run all skill self-tests
# 运行: bash run-all-tests.sh [skill-name]

set -o pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SKILLS_DIR="$SCRIPT_DIR/skills"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[0;33m'; CYAN='\033[0;36m'; NC='\033[0m'
TOTAL_PASS=0; TOTAL_FAIL=0; TOTAL_SKIP=0
SKILL_RESULTS=()

run_skill_test() {
  local skill_name=$1
  local script="$SKILLS_DIR/$skill_name/self-test.sh"
  [ ! -f "$script" ] && { echo -e "${YELLOW}⏭️  $skill_name: no self-test.sh${NC}"; SKILL_RESULTS+=("$skill_name|SKIP|no script"); return; }

  echo -e "${CYAN}▶ Testing: $skill_name${NC}"
  local start_time=$SECONDS
  local output; output=$(bash "$script" 2>&1); local exit_code=$?
  local elapsed=$(( SECONDS - start_time ))
  local pass_count=$(echo "$output" | grep -c "SELF_TEST_PASS:")
  local fail_count=$(echo "$output" | grep -c "SELF_TEST_FAIL:")
  local skip_count=$(echo "$output" | grep -c "SELF_TEST_SKIP:")

  TOTAL_PASS=$((TOTAL_PASS + pass_count)); TOTAL_FAIL=$((TOTAL_FAIL + fail_count)); TOTAL_SKIP=$((TOTAL_SKIP + skip_count))

  echo "$output" | grep "SELF_TEST_" | while read line; do
    if echo "$line" | grep -q "PASS"; then echo -e "  ${GREEN}✅ $line${NC}"
    elif echo "$line" | grep -q "FAIL"; then echo -e "  ${RED}❌ $line${NC}"
    elif echo "$line" | grep -q "SKIP"; then echo -e "  ${YELLOW}⏭️  $line${NC}"
    fi
  done

  if [ $fail_count -gt 0 ]; then
    echo -e "  ${RED}→ FAIL ($pass_count pass, $fail_count fail) [${elapsed}s]${NC}"
    SKILL_RESULTS+=("$skill_name|FAIL|$pass_count/$((pass_count+fail_count+skip_count))|${elapsed}s")
  else
    echo -e "  ${GREEN}→ PASS ($pass_count pass, $skip_count skip) [${elapsed}s]${NC}"
    SKILL_RESULTS+=("$skill_name|PASS|$pass_count/$((pass_count+skip_count))|${elapsed}s")
  fi
  echo ""
}

echo ""; echo "========================================"; echo "  Autopilot-ESP32-CAM - Skill Self-Test"; echo "========================================"; echo ""

if [ -n "$1" ]; then
  run_skill_test "$1"
else
  for skill_dir in "$SKILLS_DIR"/*/; do
    skill_name=$(basename "$skill_dir")
    [ "$skill_name" = "_common" ] && continue
    run_skill_test "$skill_name"
  done
fi

echo "========================================"; echo "  Summary"; echo "========================================"; echo ""
printf "%-25s %-8s %-12s %s\n" "Skill" "Status" "Tests" "Time"
printf "%-25s %-8s %-12s %s\n" "-------------------------" "--------" "------------" "------"
for result in "${SKILL_RESULTS[@]}"; do
  IFS='|' read -r name status tests time <<< "$result"
  case $status in PASS) sf="${GREEN}✅ PASS${NC}" ;; FAIL) sf="${RED}❌ FAIL${NC}" ;; SKIP) sf="${YELLOW}⏭️  SKIP${NC}" ;; esac
  printf "%-25s $(echo -e $sf)   %-12s %s\n" "$name" "${tests:-—}" "${time:-—}"
done
echo ""; echo -e "Total: ${GREEN}$TOTAL_PASS passed${NC}, ${RED}$TOTAL_FAIL failed${NC}, ${YELLOW}$TOTAL_SKIP skipped${NC}"; echo ""
[ $TOTAL_FAIL -gt 0 ] && { echo "Some tests FAILED."; exit 1; }
echo "All tests PASSED."; exit 0
