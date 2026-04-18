#!/bin/bash
# self-test for esp32-build-flash
# Run: bash .github/skills/esp32-build-flash/self-test.sh

PASS=0
FAIL=0
SKIP=0

test_pass() { echo "SELF_TEST_PASS: $1"; PASS=$((PASS + 1)); }
test_fail() { echo "SELF_TEST_FAIL: $1"; FAIL=$((FAIL + 1)); }
skip_case() { echo "SELF_TEST_SKIP: $1 ($2)"; SKIP=$((SKIP + 1)); }

# --- Test 1: ESP-IDF environment variables ---
if [ -n "$IDF_PATH" ]; then
  test_pass "idf_path"
else
  skip_case "idf_path" "need to source esp-idf/export.sh"
fi

# --- Test 2: idf.py executable ---
if command -v idf.py &>/dev/null; then
  test_pass "idf_cli"
else
  skip_case "idf_cli" "ESP-IDF not installed or not loaded"
fi

# --- Test 3: Compiler available ---
if command -v xtensa-esp32-elf-gcc &>/dev/null; then
  test_pass "xtensa_gcc"
elif command -v riscv32-esp-elf-gcc &>/dev/null; then
  test_pass "riscv_gcc"
else
  skip_case "cross_compiler" "need ESP-IDF toolchain"
fi

# --- Test 4: cmake available ---
if command -v cmake &>/dev/null; then
  test_pass "cmake"
else
  test_fail "cmake"
fi

# --- Test 5: ninja available ---
if command -v ninja &>/dev/null; then
  test_pass "ninja"
else
  skip_case "ninja" "brew install ninja"
fi

# --- Summary ---
echo ""
echo "Results: $PASS passed, $FAIL failed, $SKIP skipped"
exit $FAIL
