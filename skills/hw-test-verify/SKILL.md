# Skill: 硬件上板测试验证

## 用途

**每次代码变更后必须执行上板测试。** 绝不允许只编译通过就算完成。
编译通过 ≠ 功能正常，必须烧录到开发板上通过串口和浏览器验证。

**何时使用：**
- 每次修改 C 代码后
- 每次修改 HTML/JS/CSS 后（嵌入到固件中的）
- 每次修改 CMakeLists.txt 或 sdkconfig 后
- 每个任务完成时

**何时不使用：**
- 只修改了文档/日志（.md 文件）
- 只修改了 .gitignore 等非代码文件

## 强制规则

> ⛔ **如果没有成功烧录并在串口看到预期输出，该任务不算完成。**
> ⛔ **如果任务涉及 Web 功能，必须在 Chrome 浏览器中实际验证页面。**
> ⛔ **串口数据读取必须通过 tmux 下的 `idf.py monitor`，不要直接操作串口设备。**

## 串口设备检测

本项目串口芯片为 CH340，macOS 设备路径为 `/dev/cu.wchusbserial*`。

```bash
# 检测串口（支持 CH340 / CP2102 / FTDI）
SERIAL_PORT=$(ls /dev/cu.wchusbserial* /dev/cu.usbserial-* /dev/ttyUSB* 2>/dev/null | head -1)
echo "Serial port: $SERIAL_PORT"
# 期望: /dev/cu.wchusbserial110 或类似
# 如果为空 → USB 线未连接或驱动未安装
```

## 测试流程

### 1. 编译（tmux）

```bash
tmux_exec "espcam:build" "idf.py build" 300
# 必须 exit code = 0，否则停下来修 bug
# 失败时读输出:
# tmux capture-pane -t espcam:build -p -S -500 | grep "error:"
```

### 2. 烧录（tmux）

```bash
SERIAL_PORT=$(ls /dev/cu.wchusbserial* /dev/cu.usbserial-* /dev/ttyUSB* 2>/dev/null | head -1)
tmux_exec "espcam:build" "idf.py -p $SERIAL_PORT flash" 120
# 必须 exit code = 0
# 烧录失败 → 按住 BOOT 按键再试 → 检查 GPIO0/GPIO2
```

### 3. 串口监控验证（tmux + idf.py monitor）

> ⚠️ **必须使用 `idf.py monitor`** — 它提供地址解码、自动重连、彩色日志等功能。
> 不要直接用 `screen`/`minicom`/`picocom` 或 Python serial 读串口。

```bash
# 先停掉之前的 monitor（如果有）
tmux send-keys -t espcam:monitor C-]
sleep 1
tmux send-keys -t espcam:monitor C-c
sleep 1

# 启动 idf.py monitor
SERIAL_PORT=$(ls /dev/cu.wchusbserial* /dev/cu.usbserial-* /dev/ttyUSB* 2>/dev/null | head -1)
tmux send-keys -t espcam:monitor "idf.py -p $SERIAL_PORT monitor" C-m

# 等待设备启动（复位后约 3-8 秒出日志）
sleep 8

# 通过 tmux capture-pane 读取串口输出
tmux capture-pane -t espcam:monitor -p -S -500 | tail -50
```

### 4. 串口日志验证清单

从 tmux capture-pane 输出中逐项确认：

```bash
OUTPUT=$(tmux capture-pane -t espcam:monitor -p -S -500)

# 检查致命错误（任何一项匹配则必须修复）
echo "$OUTPUT" | grep -iE "Guru Meditation|abort|panic|assert failed" && echo "⛔ CRASH DETECTED" || echo "✅ No crash"

# 检查 WiFi 连接
echo "$OUTPUT" | grep -iE "sta ip:|got ip|wifi connected" || echo "⚠️ WiFi not connected"

# 提取设备 IP
DEVICE_IP=$(echo "$OUTPUT" | grep -oE 'sta ip: [0-9.]+|got ip:[0-9.]+|IP:[0-9. ]+' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | tail -1)
echo "Device IP: $DEVICE_IP"

# 检查 HTTP 服务
echo "$OUTPUT" | grep -iE "httpd|server.*start|listening" || echo "⚠️ HTTP server not started"

# 检查摄像头
echo "$OUTPUT" | grep -iE "cam_hal|camera init|sensor detected" || echo "⚠️ Camera not initialized"

# 检查内存
echo "$OUTPUT" | grep -iE "ENOMEM|alloc failed|out of memory" && echo "⛔ MEMORY ERROR" || echo "✅ Memory OK"
```

| 检查项 | 日志关键词 | 严重度 |
|--------|-----------|--------|
| 无 crash | `Guru Meditation`, `panic`, `abort` | ⛔ 必须修复 |
| WiFi 已连接 | `sta ip:`, `got ip` | ⛔ 必须成功 |
| HTTP 已启动 | `httpd`, `server`, `listening` | ⛔ 必须成功 |
| 摄像头已初始化 | `cam_hal`, `camera` | ⛔ 必须成功 |
| 无内存错误 | 无 `ENOMEM`, `alloc failed` | ⛔ 必须修复 |

### 5. Web 功能验证

#### 5a. curl 快速验证（必做）

```bash
# HTTP 首页
curl -s -o /dev/null -w "HTTP %{http_code}\n" http://$DEVICE_IP/
# 期望: HTTP 200

# MJPEG 视频流（读 3 秒检查流头）
curl -s --max-time 3 http://$DEVICE_IP/stream/tcp 2>/dev/null | head -c 200 | xxd | head -5
# 期望: JPEG 头 (ff d8 ff) 或 multipart boundary "--frame"

# LED API
curl -s http://$DEVICE_IP/api/led -X POST -d '{"state":"on"}'
# 期望: 200 + LED 物理亮起（肉眼确认 GPIO33）
```

#### 5b. Chrome 浏览器深度验证（Web 功能必做）

使用 patchright (反检测 Playwright) 打开 Chrome 实际渲染页面：

```python
# 需要 ~/patchright-env/ 或系统中安装了 patchright
from patchright.sync_api import sync_playwright
import os, time

pw = sync_playwright().start()
ctx = pw.chromium.launch_persistent_context(
    user_data_dir=os.path.expanduser("~/.patchright-userdata/espcam-verify"),
    channel='chrome',
    headless=False,  # ⛔ 必须可见模式，禁止 headless
    no_viewport=True,
)
page = ctx.pages[0] if ctx.pages else ctx.new_page()

DEVICE_IP = "REPLACE_WITH_DEVICE_IP"  # 从串口日志获取

# 验证首页
page.goto(f"http://{DEVICE_IP}/", timeout=10000)
print(f"Title: {page.title()}")

# 验证视频流
page.goto(f"http://{DEVICE_IP}/stream/tcp", timeout=10000)
time.sleep(3)
img = page.locator("img").first
if img.count():
    print(f"Stream img src: {img.get_attribute('src')}")

# 验证 HUD（FPS + 温度）
fps_ok = page.locator("text=/FPS|fps/i").count() > 0
temp_ok = page.locator("text=/°C|temperature/i").count() > 0
print(f"FPS visible: {fps_ok}, Temp visible: {temp_ok}")

# 验证 LED 按钮
led_btn = page.locator("button").filter(has_text="LED").first
if led_btn.count():
    led_btn.click()
    print("LED toggled — verify physically")

# 截图存证
page.screenshot(path="/tmp/espcam-verify.png")
print("Screenshot: /tmp/espcam-verify.png")

ctx.close()
pw.stop()
```

> 如果 patchright 不可用，回退到 5a 的 curl 验证。

### 6. 验证失败处理

```
编译失败 → tmux capture-pane 读错误 → 修代码 → 重新编译
烧录失败 → 检查串口连接 → 按住 BOOT 按键重试 → 检查 GPIO0/GPIO2
串口无输出 → 检查 TX/RX 接线 → tmux capture-pane 确认 monitor 是否启动
WiFi 连不上 → 检查 ~/.esp-wifi-credentials → 检查路由器 → 看串口错误码
panic/crash → 从 tmux capture-pane 读 backtrace → idf.py monitor 会自动解码地址
网页打不开 → 确认 IP → curl 测试 → 检查串口中 HTTP server 日志
```

## 验收标准

一个任务的"完成"必须满足以下全部条件：

- [ ] `idf.py build` 零错误零警告
- [ ] `idf.py flash` 成功烧录到开发板
- [ ] `idf.py monitor`（tmux 中）串口日志无 crash/error
- [ ] 预期功能在串口日志中可观察到
- [ ] 如涉及 Web 功能：curl + Chrome 浏览器验证通过
- [ ] git commit 记录本次变更


## Self-Test（自检）

```bash
#!/bin/bash
SKILL="skills/hw-test-verify/SKILL.md"

[ -f "$SKILL" ] && echo "SELF_TEST_PASS: skill_md_exists" || echo "SELF_TEST_FAIL: skill_md_exists"
grep -q "idf.py build" "$SKILL" && echo "SELF_TEST_PASS: has_build_step" || echo "SELF_TEST_FAIL: has_build_step"
grep -q "idf.py flash" "$SKILL" && echo "SELF_TEST_PASS: has_flash_step" || echo "SELF_TEST_FAIL: has_flash_step"
grep -q "idf.py monitor" "$SKILL" && echo "SELF_TEST_PASS: has_monitor_step" || echo "SELF_TEST_FAIL: has_monitor_step"
grep -q "curl" "$SKILL" && echo "SELF_TEST_PASS: has_curl_verify" || echo "SELF_TEST_FAIL: has_curl_verify"
grep -q "patchright" "$SKILL" && echo "SELF_TEST_PASS: has_browser_verify" || echo "SELF_TEST_FAIL: has_browser_verify"
grep -q "headless=False" "$SKILL" && echo "SELF_TEST_PASS: headless_false" || echo "SELF_TEST_FAIL: headless_false"
grep -q "tmux" "$SKILL" && echo "SELF_TEST_PASS: uses_tmux" || echo "SELF_TEST_FAIL: uses_tmux"
grep -q "CH340\|wchusbserial" "$SKILL" && echo "SELF_TEST_PASS: ch340_serial" || echo "SELF_TEST_FAIL: ch340_serial"
```

### Blind Test（盲测）

**场景描述:**
AI Agent 刚编译成功一个 ESP32-CAM Web 服务器，需要完成上板验证。

**测试 Prompt:**
> 代码已编译成功。请按照 hw-test-verify skill 完成完整的上板测试，包括烧录、串口检查和浏览器验证。

**验收标准:**
- [ ] Agent 在 tmux 中执行 idf.py flash
- [ ] Agent 用 idf.py monitor（非 screen/minicom）读取串口
- [ ] Agent 检查 WiFi 连接、HTTP 启动、无 crash
- [ ] Agent 用 curl 验证 HTTP 200
- [ ] Agent 用 patchright headless=False 打开 Chrome 验证页面
- [ ] 验证失败时 Agent 进入修复循环而非跳过

**常见失败模式:**
- Agent 只编译不烧录就继续下一个任务
- Agent 用 headless=True 或直接 screen 读串口
- Agent 忽略串口中的 warning/error 继续推进
