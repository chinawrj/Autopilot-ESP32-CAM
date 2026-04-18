---
name: esp32-build-flash
description: "ESP-IDF build and flash workflow: idf.py build, flash, monitor commands. Use when: compiling ESP32 firmware, flashing to board, checking build output."
---

# Skill: ESP32 Build and Flash

## Purpose

Manage the build, configuration, and flash workflow for ESP-IDF projects.

**When to use:**
- Need to build an ESP-IDF project
- Need to configure menuconfig options
- Need to flash firmware to an ESP32 device
- Need to clean build artifacts and rebuild

**When not to use:**
- Arduino or PlatformIO projects (use corresponding toolchains)
- Non-ESP32 platforms

## Prerequisites

- ESP-IDF v5.x installed
- Environment variables configured: `. $HOME/esp/esp-idf/export.sh`
- Target chip confirmed (ESP32 / ESP32-S2 / ESP32-S3 / ESP32-C3)

## Steps

### 1. Environment Setup

```bash
# Load ESP-IDF environment (required for each new terminal)
. $HOME/esp/esp-idf/export.sh

# Verify environment
idf.py --version
```

### 2. Project Configuration

```bash
# Set target chip
idf.py set-target esp32  # or esp32s3 etc.

# Open menu configuration
idf.py menuconfig

# Common configuration options:
# - Component config → ESP32-specific → CPU frequency (240MHz)
# - Component config → Camera configuration (ESP-CAM projects)
# - Component config → WiFi → WiFi SSID / Password
# - Serial flasher config → Flash size (4MB)
```

### 3. Build

```bash
# Full build
idf.py build

# Build app only (skip bootloader)
idf.py app

# Check build artifact sizes
idf.py size
idf.py size-components  # View by component
```

### 4. Flash

```bash
# Auto-detect port and flash
idf.py -p /dev/ttyUSB0 flash

# Flash app partition only (faster)
idf.py -p /dev/ttyUSB0 app-flash

# Flash and immediately monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

### 5. Build Error Handling

| Error Type | Common Cause | Solution |
|---------|---------|---------|
| `undefined reference` | Missing component dependency | Check REQUIRES in CMakeLists.txt |
| `region 'iram0_0_seg' overflowed` | IRAM overflow | Optimize code or enable PSRAM |
| `No such file or directory` | Incorrect header file path | Check include path configuration |
| `fatal error: esp_camera.h` | Missing camera component | Add esp32-camera component |

### 6. Clean and Rebuild

```bash
# Clean build artifacts
idf.py fullclean

# Rebuild
idf.py build
```

### 7. Automated Workflow in tmux

```bash
# Build (in build window)
tmux send-keys -t {{PROJECT_NAME}}:build '. $HOME/esp/esp-idf/export.sh && idf.py build' C-m

# Wait for build completion and check result
sleep 30  # Adjust based on project size
BUILD_OUTPUT=$(tmux capture-pane -t {{PROJECT_NAME}}:build -p | tail -20)
if echo "$BUILD_OUTPUT" | grep -q "Project build complete"; then
    echo "Build succeeded"
    # Flash
    tmux send-keys -t {{PROJECT_NAME}}:flash 'idf.py -p /dev/ttyUSB0 flash' C-m
else
    echo "Build failed, checking errors"
    tmux capture-pane -t {{PROJECT_NAME}}:build -p | grep -i "error"
fi
```

## Self-Test

> Verify that the ESP-IDF toolchain and build environment are ready.

### Self-Test Steps

```bash
# Test 1: ESP-IDF environment variables
[ -n "$IDF_PATH" ] && echo "SELF_TEST_PASS: idf_path ($IDF_PATH)" || echo "SELF_TEST_FAIL: idf_path (run . ~/esp/esp-idf/export.sh)"

# Test 2: idf.py executable
command -v idf.py &>/dev/null && echo "SELF_TEST_PASS: idf_cli" || echo "SELF_TEST_FAIL: idf_cli"

# Test 3: Compiler available
command -v xtensa-esp32-elf-gcc &>/dev/null && echo "SELF_TEST_PASS: xtensa_gcc" || echo "SELF_TEST_WARN: xtensa_gcc (may be using riscv target)"

# Test 4: Create and build minimal project
TMP_DIR=$(mktemp -d)
mkdir -p "$TMP_DIR/main"
cat > "$TMP_DIR/CMakeLists.txt" << 'EOF'
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(selftest)
EOF
cat > "$TMP_DIR/main/CMakeLists.txt" << 'EOF'
idf_component_register(SRCS "main.c" INCLUDE_DIRS ".")
EOF
cat > "$TMP_DIR/main/main.c" << 'EOF'
#include <stdio.h>
void app_main(void) { printf("selftest ok\n"); }
EOF
cd "$TMP_DIR" && idf.py set-target esp32 &>/dev/null && idf.py build &>/dev/null && \
  echo "SELF_TEST_PASS: minimal_build" || echo "SELF_TEST_FAIL: minimal_build"
rm -rf "$TMP_DIR"
```

### Expected Results

| Test Item | Expected Output | Failure Impact |
|--------|---------|----------|
| idf_path | `SELF_TEST_PASS` | Cannot use ESP-IDF |
| idf_cli | `SELF_TEST_PASS` | Cannot build/flash |
| xtensa_gcc | `SELF_TEST_PASS/WARN` | Cannot build for ESP32 target |
| minimal_build | `SELF_TEST_PASS` | Build environment has issues |

### Blind Test

**Test Prompt:**
```
You are an AI development assistant. Please read this Skill, then:
1. Check if the ESP-IDF environment is loaded
2. Create a minimal ESP32 project (only app_main printing one log line)
3. Run idf.py build and report the result
4. If the build succeeds, report the firmware size
5. Clean up temporary files
```

**Acceptance Criteria:**
- [ ] Agent loads the ESP-IDF environment first (source export.sh)
- [ ] Agent creates the correct CMakeLists.txt structure
- [ ] Agent runs build and checks the exit code
- [ ] Agent reports build success or failure reason

## Success Criteria

- [ ] `idf.py build` compiles successfully with no errors
- [ ] Firmware size is within flash capacity limits
- [ ] `idf.py flash` completes successfully
- [ ] Device outputs normal boot logs via serial after restart
