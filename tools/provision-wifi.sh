#!/bin/bash
# provision-wifi.sh — Read WiFi credentials from secure storage and inject into sdkconfig
# Usage: bash tools/provision-wifi.sh
# Credential sources (by priority):
#   1. Environment variables: ESP_WIFI_SSID / ESP_WIFI_PASSWORD
#   2. Secure file: ~/.esp-wifi-credentials

set -euo pipefail

CRED_FILE="$HOME/.esp-wifi-credentials"

# Prefer environment variables
if [[ -n "${ESP_WIFI_SSID:-}" && -n "${ESP_WIFI_PASSWORD:-}" ]]; then
    SSID="$ESP_WIFI_SSID"
    PASS="$ESP_WIFI_PASSWORD"
    echo "📡 Using WiFi credentials from environment variables"

# Fall back to reading from file
elif [[ -f "$CRED_FILE" ]]; then
    SSID=$(grep -E "^ssid\s*=" "$CRED_FILE" | sed 's/^ssid[[:space:]]*=[[:space:]]*//')
    PASS=$(grep -E "^password\s*=" "$CRED_FILE" | sed 's/^password[[:space:]]*=[[:space:]]*//')
    echo "📡 Using WiFi credentials from $CRED_FILE"
else
    echo "❌ No WiFi credentials found!"
    echo ""
    echo "Option 1: Create credentials file"
    echo "  cat > ~/.esp-wifi-credentials << 'EOF'"
    echo "  [wifi]"
    echo "  ssid = YOUR_SSID"
    echo "  password = YOUR_PASSWORD"
    echo "  EOF"
    echo "  chmod 600 ~/.esp-wifi-credentials"
    echo ""
    echo "Option 2: Set environment variables"
    echo "  export ESP_WIFI_SSID=YOUR_SSID"
    echo "  export ESP_WIFI_PASSWORD=YOUR_PASSWORD"
    exit 1
fi

if [[ -z "${SSID:-}" || -z "${PASS:-}" ]]; then
    echo "❌ SSID or password is empty in $CRED_FILE!"
    exit 1
fi

# Write to sdkconfig (this file is already in .gitignore)
# If sdkconfig exists, remove old WiFi config first
if [[ -f sdkconfig ]]; then
    sed -i.bak '/^CONFIG_ESP_WIFI_SSID=/d; /^CONFIG_ESP_WIFI_PASSWORD=/d' sdkconfig
    rm -f sdkconfig.bak
fi

echo "CONFIG_ESP_WIFI_SSID=\"$SSID\"" >> sdkconfig
echo "CONFIG_ESP_WIFI_PASSWORD=\"$PASS\"" >> sdkconfig

echo "✅ WiFi credentials injected into sdkconfig"
echo "   SSID: $SSID"
echo "   Password: ****$(echo "$PASS" | tail -c 5)"
echo ""
echo "Now run: idf.py build && idf.py flash"
