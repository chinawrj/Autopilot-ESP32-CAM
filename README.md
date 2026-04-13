# Autopilot ESP32-CAM

基于 **YD-ESP32-CAM** 开发板的实时摄像头 Web 服务器，支持 TCP/WebSocket 双路径视频流、
实时 HUD 叠加显示、LED 远程控制。由 AI Agent 以每日迭代方式从零开发。

## 功能特性

- **TCP 视频流** (`/stream/tcp`) — MJPEG over HTTP，浏览器直接 `<img>` 标签播放
- **WebSocket 视频流** (`/stream/udp`) — 二进制 JPEG 帧推送 + Canvas 渲染，支持最多 4 客户端
- **实时 HUD** — FPS 计数器 + 虚拟温度传感器 (25°C ±3°C)，前端叠加显示
- **WebSocket 控制** — 动态调整画质 (Q10-Q50)、分辨率 (QVGA/VGA/SVGA/XGA)
- **LED 控制** — 网页按钮控制板载 LED (GPIO33) 开/关/切换
- **心跳机制** — 5 秒周期心跳，推送 FPS/客户端数/堆内存等状态
- **WiFi 自动重连** — 断线后指数退避无限重连 (1s → 10s)
- **堆内存监控** — `/api/status` 返回实时堆内存信息，每 30s 串口日志输出

## 硬件

| 参数 | 值 |
|------|-----|
| 开发板 | YD-ESP32-CAM (源地工作室 VCC-GND Studio) |
| 核心模组 | ESP32-WROVER-E-N8R8 (8MB Flash + 8MB PSRAM) |
| 芯片 | ESP32-D0WD-V3 (双核 Xtensa LX6, 240MHz) |
| 摄像头 | OV2640 (VGA 640×480, JPEG q=12) |
| 板载 LED | GPIO33 |
| 串口芯片 | CH340 |

## 项目结构

```
├── main/
│   ├── main.c              # 入口，初始化链 + 堆日志
│   ├── wifi_manager.c/h    # WiFi STA 管理，自动重连
│   ├── camera_init.c/h     # OV2640 摄像头初始化
│   ├── http_server.c/h     # HTTP 服务器，路由注册
│   ├── ws_stream.c/h       # WebSocket 视频流 + 控制消息
│   ├── led_controller.c/h  # GPIO33 LED 驱动
│   ├── index.html          # MJPEG 流前端页面
│   └── stream_udp.html     # WebSocket 流前端页面
├── components/
│   └── virtual_sensor/     # 虚拟温度传感器组件
├── tools/
│   ├── provision-wifi.sh   # WiFi 凭据安全注入
│   ├── heap_monitor.py     # 堆内存趋势监控工具
│   ├── multi_client_test.py # 多客户端并发压力测试
│   └── browser_verify.py   # 浏览器自动化验证
├── docs/
│   ├── TARGET.md           # 里程碑进度跟踪
│   └── daily-logs/         # 每日开发日志
├── sdkconfig.defaults      # ESP-IDF 默认配置
├── partitions.csv          # 分区表 (3MB app + 960KB storage)
└── CMakeLists.txt
```

## 快速开始

### 1. 环境准备

```bash
# ESP-IDF v5.x 环境
. $HOME/esp/esp-idf/export.sh
```

### 2. 配置 WiFi 凭据

WiFi 密码**不存储在仓库中**。使用以下方式注入：

```bash
# 方式一：环境变量
export ESP_WIFI_SSID="YourSSID"
export ESP_WIFI_PASSWORD="YourPassword"

# 方式二：安全配置文件 (推荐)
cat > ~/.esp-wifi-credentials << 'EOF'
[wifi]
ssid = YourSSID
password = YourPassword
EOF
chmod 600 ~/.esp-wifi-credentials

# 注入凭据到构建配置
bash tools/provision-wifi.sh
```

### 3. 编译与烧录

```bash
idf.py build
idf.py -p /dev/cu.wchusbserial110 flash monitor
```

### 4. 访问 Web 界面

设备连网后，串口会输出 IP 地址：

```
I (2380) wifi_mgr: WiFi connected, IP: 192.168.1.171
I (2630) main: System ready — http://192.168.1.171/
```

| 页面 | URL | 说明 |
|------|-----|------|
| MJPEG 流 | `http://<IP>/` | TCP MJPEG 视频流 + HUD |
| WebSocket 流 | `http://<IP>/stream/udp` | WebSocket 视频流 + 控制面板 |
| 状态 API | `http://<IP>/api/status` | JSON: fps, temperature, heap 等 |
| LED 控制 | `POST http://<IP>/api/led` | Body: `{"state":"on/off/toggle"}` |

## API 说明

### GET `/api/status`

```json
{
  "fps": 10.5,
  "temperature": 25.3,
  "led_state": false,
  "heap_free": 4224764,
  "heap_min": 4161592
}
```

### POST `/api/led`

```bash
curl -X POST http://192.168.1.171/api/led -d '{"state":"toggle"}'
```

### WebSocket `/ws/stream`

二进制帧：JPEG 图像数据
文本帧（心跳）：
```json
{
  "type": "heartbeat",
  "fps": 10.5,
  "clients": 2,
  "heap_free": 4224764,
  "heap_min": 4161592
}
```

控制消息（客户端 → 服务器）：
```json
{"action": "set_quality", "value": 20}
{"action": "set_resolution", "value": "SVGA"}
{"action": "get_status"}
```

## 性能指标

| 指标 | 值 |
|------|-----|
| MJPEG FPS | ~10 fps (VGA, 1 客户端) |
| WebSocket FPS | ~10 fps (VGA, 1 客户端) |
| 多客户端 | 2 WS + 1 MJPEG 同时稳定 |
| JPEG 帧大小 | ~10-15 KB (VGA, q=12) |
| 堆内存空闲 | ~4.1 MB (含 PSRAM) |
| 固件大小 | ~1034 KB (Flash 67% 空闲) |
| WiFi 重连 | 自动，1-10s 指数退避 |

## 开发框架

- **ESP-IDF v5.x** — Espressif 官方 IoT 开发框架
- **HTTP Server** — ESP-IDF `esp_http_server` 组件
- **WebSocket** — ESP-IDF httpd WebSocket 扩展
- **Camera** — `esp32-camera` 驱动
- **cJSON** — JSON 解析/生成

## 许可

MIT

### 3. 访问

打开浏览器访问 `http://<ESP32-IP>/`

## 项目结构

```
├── .github/
│   ├── copilot-instructions.md    # Copilot 开发指令
│   └── esp-cam-dev.agent.md       # 开发 Agent 定义
├── main/                           # ESP-IDF 主组件 (待创建)
├── components/                     # 自定义组件 (待创建)
├── frontend/                       # Web 前端 (待创建)
├── tools/
│   └── provision-wifi.sh           # WiFi 凭据注入工具
├── skills/
│   └── wifi-credentials/           # WiFi 安全管理 Skill
├── docs/
│   ├── TARGET.md                   # 里程碑与进度跟踪
│   └── daily-logs/                 # 每日工作日志
│       ├── TEMPLATE.md
│       └── day-000.md
└── README.md
```

## 开发方式

本项目由 AI Agent 以**每日迭代**模式自主开发：
- 每次对话 = 1 个工作日
- 每天完成 2-3 个具体可验证的任务
- 详细日志记录在 `docs/daily-logs/`
- 进度跟踪在 `docs/TARGET.md`

## License

MIT
