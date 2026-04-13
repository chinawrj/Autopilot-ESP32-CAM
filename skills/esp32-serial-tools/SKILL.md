# Skill: ESP32 串口工具

## 用途

管理 ESP32 设备的串口通信，包括日志监控、数据解析、异常检测。

**何时使用：**
- 需要监控 ESP32 设备的串口输出
- 需要解析串口日志中的关键信息
- 需要检测运行时错误（panic, assert, watchdog）
- 需要通过串口发送命令到设备

**何时不使用：**
- 不涉及串口通信的项目
- 仅需要 Web 接口交互的场景

## 前置条件

- ESP-IDF 已安装并配置环境变量
- USB 串口驱动已安装（CP2102/CH340）
- 设备已通过 USB 连接

## 操作步骤

### 1. 识别串口设备

```bash
# macOS
ls /dev/tty.usb* /dev/cu.usb* 2>/dev/null

# Linux
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null

# 通常结果：/dev/ttyUSB0 或 /dev/cu.usbserial-xxx
```

### 2. 启动串口监控

```bash
# 方式一：使用 idf.py monitor（推荐）
idf.py -p /dev/ttyUSB0 monitor

# 方式二：使用 minicom
minicom -D /dev/ttyUSB0 -b 115200

# 方式三：使用 screen
screen /dev/ttyUSB0 115200
```

### 3. 日志捕获与分析

在 tmux 环境中，可以自动捕获和分析日志：

```bash
# 捕获 monitor 窗口的最新输出
tmux capture-pane -t {{PROJECT_NAME}}:monitor -p | tail -50

# 检查是否有错误
tmux capture-pane -t {{PROJECT_NAME}}:monitor -p | grep -iE "(error|panic|assert|abort|watchdog)"

# 检查 WiFi 连接状态
tmux capture-pane -t {{PROJECT_NAME}}:monitor -p | grep -iE "(wifi|connected|ip addr|got ip)"

# 检查 HTTP 服务器状态
tmux capture-pane -t {{PROJECT_NAME}}:monitor -p | grep -iE "(httpd|server|listening|port)"
```

### 4. 常见日志模式识别

| 日志模式 | 含义 | 处理方式 |
|---------|------|---------|
| `Guru Meditation Error` | CPU 异常（panic） | 检查 backtrace，定位崩溃代码 |
| `Task watchdog got triggered` | 任务看门狗超时 | 检查是否有死循环或阻塞 |
| `Wi-Fi connected` | WiFi 已连接 | 正常，记录 IP 地址 |
| `httpd_start: Started` | HTTP 服务已启动 | 检查端口号 |
| `cam_hal: cam_dma_config` | 摄像头初始化 | 检查分辨率和帧率 |
| `ENOMEM` / `alloc failed` | 内存不足 | 降低分辨率或减少缓冲区 |

### 5. 串口数据提取脚本

```python
import serial
import re
import time

def monitor_serial(port="/dev/ttyUSB0", baudrate=115200, timeout=30):
    """监控串口输出，提取关键信息"""
    results = {
        "ip_address": None,
        "errors": [],
        "wifi_connected": False,
        "http_started": False,
    }

    ser = serial.Serial(port, baudrate, timeout=1)
    start = time.time()

    while time.time() - start < timeout:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        if not line:
            continue

        # 提取 IP 地址
        ip_match = re.search(r"got ip:(\d+\.\d+\.\d+\.\d+)", line)
        if ip_match:
            results["ip_address"] = ip_match.group(1)

        # 检查 WiFi 连接
        if "wifi connected" in line.lower() or "sta_connected" in line.lower():
            results["wifi_connected"] = True

        # 检查 HTTP 服务
        if "httpd" in line.lower() and "start" in line.lower():
            results["http_started"] = True

        # 收集错误
        if re.search(r"error|panic|assert|abort", line, re.IGNORECASE):
            results["errors"].append(line)

    ser.close()
    return results
```

## Self-Test（自检）

> 验证串口工具和 Python 串口库可用。

### 自检步骤

```bash
# Test 1: pyserial 可导入
python3 -c "import serial; print('SELF_TEST_PASS: pyserial')" 2>/dev/null || echo "SELF_TEST_FAIL: pyserial"

# Test 2: 串口设备检测
PORTS=$(ls /dev/tty.usb* /dev/cu.usb* /dev/ttyUSB* /dev/ttyACM* 2>/dev/null)
if [ -n "$PORTS" ]; then
    echo "SELF_TEST_PASS: serial_device ($PORTS)"
else
    echo "SELF_TEST_WARN: serial_device (无设备连接，需要硬件）"
fi

# Test 3: idf.py monitor 命令存在（需要 ESP-IDF 环境）
command -v idf.py &>/dev/null && echo "SELF_TEST_PASS: idf_monitor" || echo "SELF_TEST_WARN: idf_monitor (ESP-IDF 未加载)"

# Test 4: 日志模式匹配逻辑验证
python3 -c "
import re
test_lines = [
    'I (1234) wifi: got ip:192.168.1.100',
    'E (5678) app: Guru Meditation Error',
    'I (9999) httpd: httpd_start: Started',
]
for line in test_lines:
    if re.search(r'got ip:(\d+\.\d+\.\d+\.\d+)', line):
        print(f'  IP extracted: {re.search(r"got ip:(\d+\.\d+\.\d+\.\d+)", line).group(1)}')
    if re.search(r'error|panic|assert|abort', line, re.IGNORECASE):
        print(f'  Error detected: {line.strip()}')
print('SELF_TEST_PASS: pattern_matching')
" || echo "SELF_TEST_FAIL: pattern_matching"
```

### 预期结果

| 测试项 | 预期输出 | 失败影响 |
|--------|---------|----------|
| pyserial | `SELF_TEST_PASS` | Python 串口脚本不可用 |
| serial_device | `SELF_TEST_PASS/WARN` | 需要连接硬件 |
| idf_monitor | `SELF_TEST_PASS/WARN` | 可用 minicom 替代 |
| pattern_matching | `SELF_TEST_PASS` | 日志分析功能异常 |

### Blind Test（盲测）

**测试 Prompt:**
```
你是一个 AI 开发助手。请阅读此 Skill，然后：
1. 检查当前系统是否有串口设备连接
2. 编写一个 Python 脚本，解析以下模拟串口输出并提取关键信息：
   - "I (1000) wifi: connected to ap SSID:MyWiFi"
   - "I (2000) wifi: got ip:192.168.1.50"
   - "I (3000) httpd: httpd_start: Started on port 80"
   - "E (4000) cam: cam_hal: failed to init"
3. 输出提取到的 IP 地址、WiFi 状态、HTTP 状态和错误列表
```

**验收标准:**
- [ ] Agent 正确使用 Skill 中的正则模式
- [ ] Agent 提取到 IP: 192.168.1.50
- [ ] Agent 识别到 cam_hal 错误
- [ ] Agent 没有遗漏 httpd 启动信息

## 成功标准

- [ ] 能正确识别并连接串口设备
- [ ] 串口日志实时输出可见
- [ ] 能自动检测常见错误模式
- [ ] 能提取关键信息（IP 地址、服务状态等）
