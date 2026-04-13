# Autopilot ESP32-CAM

AI Agent 自动驾驶开发的 ESP32 摄像头 Web 服务器项目。

## 硬件

| 参数 | 值 |
|------|-----|
| 开发板 | YD-ESP32-CAM (源地工作室 VCC-GND Studio) |
| 核心模组 | ESP32-WROVER-E-N8R8 (8MB Flash + 8MB PSRAM) |
| 摄像头 | OV2640 |
| 板载 LED | GPIO33 |

## 功能目标

- [x] WiFi 连接（凭据安全管理）
- [ ] TCP 视频流 (`/stream/tcp`) — MJPEG over HTTP
- [ ] UDP 视频流 (`/stream/udp`) — UDP 推送 + WebSocket 控制
- [ ] 实时 HUD — FPS 计数器 + 虚拟温度传感器
- [ ] LED 控制 — 网页按钮控制 GPIO33

## 快速开始

### 1. 环境准备

```bash
# ESP-IDF 环境
. $HOME/esp/esp-idf/export.sh

# 配置 WiFi 凭据 (仓库外安全存储)
cat > ~/.esp-wifi-credentials << 'CRED'
[wifi]
ssid = YOUR_SSID
password = YOUR_PASSWORD
CRED
chmod 600 ~/.esp-wifi-credentials
```

### 2. 编译与烧录

```bash
# 注入 WiFi 凭据
bash tools/provision-wifi.sh

# 编译
idf.py build

# 烧录 (替换为实际串口)
idf.py -p /dev/cu.usbserial-* flash monitor
```

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
