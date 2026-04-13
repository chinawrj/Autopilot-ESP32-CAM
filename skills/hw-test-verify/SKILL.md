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
> ⛔ **如果任务涉及 Web 功能，必须在浏览器中实际验证页面。**

## 测试流程

### 1. 编译 → 烧录 → 串口验证（每次代码变更必做）

```bash
# 使用 tmux（参见 skills/tmux-session/SKILL.md）

# Step 1: 编译
tmux_exec "espcam:build" "idf.py build" 300
# 必须 exit code = 0，否则停下来修 bug

# Step 2: 烧录
SERIAL_PORT=$(ls /dev/cu.usbserial-* /dev/ttyUSB* 2>/dev/null | head -1)
tmux_exec "espcam:build" "idf.py -p $SERIAL_PORT flash" 120
# 必须 exit code = 0

# Step 3: 串口监控（验证启动日志）
tmux send-keys -t espcam:monitor C-c  # 先停掉之前的 monitor
sleep 1
tmux send-keys -t espcam:monitor "idf.py -p $SERIAL_PORT monitor" C-m
sleep 8  # 等待设备启动

# Step 4: 检查关键日志
tmux capture-pane -t espcam:monitor -p -S -500 | grep -iE "(wifi|connected|ip|error|panic|http|started|cam)"
```

### 2. 串口日志验证清单

每次上板后，从串口输出中确认以下信息：

| 检查项 | 日志关键词 | 说明 |
|--------|-----------|------|
| 无 panic/crash | `Guru Meditation`, `abort()`, `panic` | 任何 crash 必须立即修复 |
| WiFi 连接 | `sta ip:`, `connected`, `got ip` | 记录分配的 IP 地址 |
| HTTP 启动 | `httpd`, `server`, `listening` | 确认 Web 服务器启动 |
| 摄像头初始化 | `cam_hal`, `camera` | 确认摄像头正常 |
| 内存充足 | 无 `ENOMEM`, `alloc failed` | 内存不足需降分辨率 |

### 3. Web 功能浏览器验证（涉及 HTTP 功能时必做）

```bash
# 从串口日志提取设备 IP
DEVICE_IP=$(tmux capture-pane -t espcam:monitor -p -S -500 | grep -oE 'sta ip: [0-9.]+|got ip:[0-9.]+|IP: [0-9.]+' | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | tail -1)
echo "Device IP: $DEVICE_IP"

# 验证 HTTP 响应
curl -s -o /dev/null -w "%{http_code}" http://$DEVICE_IP/
# 期望: 200

# 验证视频流头
curl -s --max-time 3 http://$DEVICE_IP/stream/tcp | head -c 200 | xxd | head -5
# 期望: 看到 JPEG 头 (ff d8 ff) 或 multipart boundary

# 验证 API
curl -s http://$DEVICE_IP/api/led -X POST -d '{"state":"on"}'
# 期望: 200 + LED 物理亮起
```

### 4. 验证失败处理

```
编译失败 → 读错误日志 → 修代码 → 重新编译（不要跳过）
烧录失败 → 检查串口连接 → 按住 BOOT 按键重试 → 检查 GPIO0/GPIO2
串口无输出 → 检查 TX/RX 接线 → 换波特率 → 重新烧录
WiFi 连不上 → 检查 ~/.esp-wifi-credentials → 检查路由器
panic/crash → 读 backtrace → 定位代码行 → 修复
网页打不开 → 确认 IP → 检查防火墙 → 检查 HTTP server 日志
```

## 验收标准

一个任务的"完成"必须满足以下全部条件：

- [ ] `idf.py build` 零错误零警告
- [ ] `idf.py flash` 成功烧录
- [ ] 串口日志无 panic/crash/error
- [ ] 预期功能在串口中可观察到（WiFi 连接、HTTP 启动等）
- [ ] 如涉及 Web 功能：浏览器或 curl 验证通过
- [ ] git commit 记录本次变更
