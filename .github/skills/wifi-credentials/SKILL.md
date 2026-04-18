---
name: wifi-credentials
description: "WiFi credential security management: external credential file, provision script, .gitignore rules. Use when: configuring WiFi, managing SSID/password, preventing credential leaks."
---

# Skill: WiFi Credential Security Management

## Purpose

Securely manage WiFi SSID and password for ESP32 projects, ensuring sensitive information is never leaked into the Git repository.

**When to use:**
- The project requires WiFi connectivity
- Setting up the development environment for the first time
- Switching to a different target router

**When not to use:**
- Projects that do not involve WiFi
- Scenarios using enterprise certificate authentication

## Prerequisites

- ESP-IDF is installed
- Target router SSID and password are known

### Credential Storage Location

Credentials are stored at a **fixed location outside the repository**, not tracked by Git:

```
~/.esp-wifi-credentials
```

### File Format

```ini
# ESP32 WiFi credentials — this file must not be added to any Git repository
[wifi]
ssid = YOUR_SSID_HERE
password = YOUR_PASSWORD_HERE
```

## Steps

### 1. Create the Credential File

```bash
# Create the credential file (first time only)
cat > ~/.esp-wifi-credentials << 'CRED'
[wifi]
ssid = YOUR_SSID_HERE
password = YOUR_PASSWORD_HERE
CRED

# Set strict permissions (readable only by the current user)
chmod 600 ~/.esp-wifi-credentials

echo "✅ Credential file created: ~/.esp-wifi-credentials"
echo "⚠️  Please edit this file to fill in your actual SSID and password"
```

### 2. Inject Credentials Before Flashing

Use the `tools/provision-wifi.sh` script to write credentials into sdkconfig before flashing:

```bash
#!/bin/bash
# tools/provision-wifi.sh — Read WiFi credentials from the secure file and inject into build config

CRED_FILE="$HOME/.esp-wifi-credentials"

# Prefer environment variables
if [[ -n "$ESP_WIFI_SSID" && -n "$ESP_WIFI_PASSWORD" ]]; then
    SSID="$ESP_WIFI_SSID"
    PASS="$ESP_WIFI_PASSWORD"
    echo "📡 Using WiFi credentials from environment variables"

# Fall back to reading from file
elif [[ -f "$CRED_FILE" ]]; then
    SSID=$(grep -E "^ssid\s*=" "$CRED_FILE" | sed 's/^ssid\s*=\s*//')
    PASS=$(grep -E "^password\s*=" "$CRED_FILE" | sed 's/^password\s*=\s*//')
    echo "📡 Using WiFi credentials from $CRED_FILE"
else
    echo "❌ No WiFi credentials found!"
    echo "   Create ~/.esp-wifi-credentials or set ESP_WIFI_SSID/ESP_WIFI_PASSWORD"
    exit 1
fi

if [[ -z "$SSID" || -z "$PASS" ]]; then
    echo "❌ SSID or password is empty!"
    exit 1
fi

# Write to sdkconfig (not tracked by git, already in .gitignore)
echo "CONFIG_ESP_WIFI_SSID=\"$SSID\"" >> sdkconfig
echo "CONFIG_ESP_WIFI_PASSWORD=\"$PASS\"" >> sdkconfig

echo "✅ WiFi credentials injected into sdkconfig"
echo "   SSID: $SSID"
echo "   Password: ****$(echo "$PASS" | tail -c 4)"
```

### 3. ESP-IDF Kconfig Definition

Define WiFi configuration options in `main/Kconfig.projbuild`:

```
menu "WiFi Configuration"

    config ESP_WIFI_SSID
        string "WiFi SSID"
        default "DEFAULT_SSID"
        help
            WiFi network name. Override via menuconfig or provision-wifi.sh.

    config ESP_WIFI_PASSWORD
        string "WiFi Password"
        default "DEFAULT_PASS"
        help
            WiFi password. Override via menuconfig or provision-wifi.sh.

    config ESP_MAXIMUM_RETRY
        int "Maximum retry"
        default 5
        help
            Maximum number of retries before giving up WiFi connection.

endmenu
```

### 4. Usage in C Code

```c
// wifi_manager.c
#include "esp_wifi.h"

#define WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define WIFI_MAX_RETRY CONFIG_ESP_MAXIMUM_RETRY
```

### 5. .gitignore Security Rules

Ensure the following entries are in `.gitignore`:

```gitignore
# WiFi credential related — never commit
sdkconfig
sdkconfig.old
*.credentials
**/wifi_config.h

# But keep the defaults template
!sdkconfig.defaults
```

### Verification Checklist

- [ ] `~/.esp-wifi-credentials` exists with permissions set to 600
- [ ] `sdkconfig` is in `.gitignore`
- [ ] No hardcoded SSID/password strings in source code
- [ ] `tools/provision-wifi.sh` can correctly read and inject credentials
- [ ] `git log --all -p | grep -i "password"` shows no real password leaks

## Self-Test

```bash
#!/bin/bash
PASS=0; FAIL=0

test_case() {
    local name=$1; shift
    if "$@" 2>/dev/null; then
        echo "SELF_TEST_PASS: $name"; PASS=$((PASS+1))
    else
        echo "SELF_TEST_FAIL: $name"; FAIL=$((FAIL+1))
    fi
}

# Credential file format is correct
test_case "cred_file_location" test -n "$HOME/.esp-wifi-credentials"

# .gitignore contains sdkconfig
test_case "gitignore_sdkconfig" grep -q "sdkconfig" .gitignore

# No hardcoded passwords in source code (search common patterns)
test_case "no_hardcoded_password" bash -c '! grep -rn "NETGEAR\|password.*=.*\"[a-zA-Z0-9]" main/ components/ 2>/dev/null | grep -v Kconfig | grep -v "CONFIG_"'

echo "Results: $PASS passed, $FAIL failed"
exit $FAIL
```

### Blind Test

**Scenario:**
The AI Agent needs to configure WiFi connectivity for the ESP32-CAM project, with known target router information.

**Test Prompt:**
> Please help me configure the project WiFi connection. The router SSID is MyRouter and the password is secret123.

**Acceptance Criteria:**
- [ ] Agent does not write the password to any Git-tracked file
- [ ] Agent creates or guides creation of `~/.esp-wifi-credentials`
- [ ] Agent uses the Kconfig + provision script approach
- [ ] Agent prompts the user to manually edit the credential file

**Common Failure Modes:**
- Agent writes the password directly into .c files or sdkconfig.defaults → serious security issue
- Agent ignores file permission settings → password may be readable by other users


## Success Criteria

- [ ] Credential file `~/.esp-wifi-credentials` exists with permissions 600
- [ ] `sdkconfig` is in `.gitignore`
- [ ] No hardcoded SSID/password in source code
- [ ] Provision script can correctly inject credentials
- [ ] No password leaks in git history
