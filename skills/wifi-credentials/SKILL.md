# Skill: WiFi 凭据安全管理

## 用途

安全地管理 ESP32 项目的 WiFi SSID 和密码，确保敏感信息不会泄漏到 Git 仓库中。

**何时使用：**
- 项目需要 WiFi 连接
- 首次配置开发环境
- 更换目标路由器

**何时不使用：**
- 不涉及 WiFi 的项目
- 使用企业级证书认证的场景

## 前置条件

- ESP-IDF 已安装
- 目标路由器 SSID 和密码已知

## 凭据存储位置

凭据存储在**仓库之外的固定位置**，不被 Git 跟踪：

```
~/.esp-wifi-credentials
```

### 文件格式

```ini
# ESP32 WiFi 凭据 — 此文件不应加入任何 Git 仓库
[wifi]
ssid = YOUR_SSID_HERE
password = YOUR_PASSWORD_HERE
```

## 操作步骤

### 1. 创建凭据文件

```bash
# 创建凭据文件（仅首次）
cat > ~/.esp-wifi-credentials << 'CRED'
[wifi]
ssid = YOUR_SSID_HERE
password = YOUR_PASSWORD_HERE
CRED

# 设置严格权限（仅当前用户可读）
chmod 600 ~/.esp-wifi-credentials

echo "✅ 凭据文件已创建: ~/.esp-wifi-credentials"
echo "⚠️  请编辑此文件填入实际的 SSID 和密码"
```

### 2. 烧录前注入凭据

使用 `tools/provision-wifi.sh` 脚本，在烧录前将凭据写入 sdkconfig：

```bash
#!/bin/bash
# tools/provision-wifi.sh — 从安全文件读取 WiFi 凭据并注入构建配置

CRED_FILE="$HOME/.esp-wifi-credentials"

# 优先使用环境变量
if [[ -n "$ESP_WIFI_SSID" && -n "$ESP_WIFI_PASSWORD" ]]; then
    SSID="$ESP_WIFI_SSID"
    PASS="$ESP_WIFI_PASSWORD"
    echo "📡 Using WiFi credentials from environment variables"

# 其次从文件读取
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

# 写入 sdkconfig (不会被 git 跟踪，已在 .gitignore 中)
echo "CONFIG_ESP_WIFI_SSID=\"$SSID\"" >> sdkconfig
echo "CONFIG_ESP_WIFI_PASSWORD=\"$PASS\"" >> sdkconfig

echo "✅ WiFi credentials injected into sdkconfig"
echo "   SSID: $SSID"
echo "   Password: ****$(echo "$PASS" | tail -c 4)"
```

### 3. ESP-IDF Kconfig 定义

在 `main/Kconfig.projbuild` 中定义 WiFi 配置项：

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

### 4. C 代码中使用

```c
// wifi_manager.c
#include "esp_wifi.h"

#define WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define WIFI_MAX_RETRY CONFIG_ESP_MAXIMUM_RETRY
```

### 5. .gitignore 安全规则

确保以下条目在 `.gitignore` 中：

```gitignore
# WiFi 凭据相关 — 绝不提交
sdkconfig
sdkconfig.old
*.credentials
**/wifi_config.h

# 但保留 defaults 模板
!sdkconfig.defaults
```

## 验证清单

- [ ] `~/.esp-wifi-credentials` 存在且权限为 600
- [ ] `sdkconfig` 在 `.gitignore` 中
- [ ] 源代码中无硬编码的 SSID/密码字符串
- [ ] `tools/provision-wifi.sh` 可正确读取并注入凭据
- [ ] `git log --all -p | grep -i "password"` 无真实密码泄漏

## Self-Test（自检）

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

# 凭据文件格式正确
test_case "cred_file_location" test -n "$HOME/.esp-wifi-credentials"

# .gitignore 包含 sdkconfig
test_case "gitignore_sdkconfig" grep -q "sdkconfig" .gitignore

# 源代码中无硬编码密码 (搜索常见模式)
test_case "no_hardcoded_password" bash -c '! grep -rn "NETGEAR\|password.*=.*\"[a-zA-Z0-9]" main/ components/ 2>/dev/null | grep -v Kconfig | grep -v "CONFIG_"'

echo "Results: $PASS passed, $FAIL failed"
exit $FAIL
```

### Blind Test（盲测）

**场景描述:**
AI Agent 需要为 ESP32-CAM 项目配置 WiFi 连接，已知目标路由器信息。

**测试 Prompt:**
> 请帮我配置项目 WiFi 连接，路由器 SSID 是 MyRouter，密码是 secret123。

**验收标准:**
- [ ] Agent 不将密码写入任何 Git 跟踪的文件
- [ ] Agent 创建或引导创建 `~/.esp-wifi-credentials`
- [ ] Agent 使用 Kconfig + provision 脚本方案
- [ ] 提示用户手动编辑凭据文件

**常见失败模式:**
- Agent 直接将密码写入 .c 文件或 sdkconfig.defaults → 严重安全问题
- Agent 忽略文件权限设置 → 密码可能被其他用户读取


## 成功标准

- [ ] 凭据文件 `~/.esp-wifi-credentials` 存在且权限 600
- [ ] `sdkconfig` 在 `.gitignore` 中
- [ ] 源代码中无硬编码 SSID/密码
- [ ] provision 脚本可正确注入凭据
- [ ] git 历史中无密码泄漏
