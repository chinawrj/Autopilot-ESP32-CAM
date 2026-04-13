# v1.0.0 — Initial Release | 首个正式版本

## 🎉 Autopilot ESP32-CAM v1.0.0

A complete real-time camera web server for the **YD-ESP32-CAM** (ESP32-WROVER-E-N8R8) development board, developed entirely by an AI Agent through 12 daily iterations.

基于 **YD-ESP32-CAM** (ESP32-WROVER-E-N8R8) 开发板的完整实时摄像头 Web 服务器，由 AI Agent 通过 12 天每日迭代从零开发至交付。

---

## ✨ Features | 功能特性

### Video Streaming | 视频流
- **TCP MJPEG Stream** (`/stream/tcp`) — HTTP multipart MJPEG, works in any browser `<img>` tag
- **WebSocket Stream** (`/ws/stream`) — Binary JPEG push + Canvas rendering, up to **4 concurrent clients**
- Dynamic **quality control** (Q10–Q50) and **resolution control** (QVGA / VGA / SVGA / XGA) via WebSocket

### HUD & Monitoring | 实时显示与监控
- **Real-time HUD Overlay** — FPS counter + virtual temperature sensor (25°C ±3°C)
- **Heartbeat** — 5-second periodic status push (FPS, client count, heap memory)
- **Status API** (`GET /api/status`) — JSON with fps, temperature, LED state, heap info

### Control | 控制
- **LED Remote Control** — Web button and REST API (`POST /api/led`) for GPIO33 on/off/toggle
- **WiFi Auto-Reconnect** — Exponential backoff (1s → 10s), infinite retry after initial connect

### Documentation | 文档
- **Bilingual README** — English ([README.md](README.md)) + Chinese ([README_CN.md](README_CN.md))
- **Architecture Diagram** — Mermaid-based system diagram embedded in README
- **Screenshots** — Captured from real hardware via browser automation

---

## 📊 Performance | 性能指标

| Metric | Value |
|--------|-------|
| MJPEG FPS | ~10 fps (VGA, single client) |
| WebSocket FPS | ~10 fps (VGA, single client) |
| Multi-client | 2 WS + 1 MJPEG simultaneous, **0 errors** |
| JPEG Frame Size | ~10-15 KB (VGA, q=12) |
| Free Heap | ~4.1 MB (with PSRAM) |
| Firmware Size | ~1034 KB (67% Flash free) |
| Total C Code | ~850 lines across 7 source files |

---

## 🔧 Hardware | 硬件

| Parameter | Value |
|-----------|-------|
| Board | YD-ESP32-CAM (VCC-GND Studio) |
| Module | ESP32-WROVER-E-N8R8 (8MB Flash + 8MB PSRAM) |
| SoC | ESP32-D0WD-V3 (Dual-core Xtensa LX6, 240MHz) |
| Camera | OV2640 (AI-Thinker compatible pinout) |
| Onboard LED | GPIO33 |

---

## 🧪 Testing | 测试验证

All tests passed on real hardware:

| Test | Result |
|------|--------|
| Multi-client stress (2WS + 1MJPEG, 60s) | ✅ 0 errors |
| WiFi disconnect/reconnect (4 rounds) | ✅ 4/4 success, ~2s recovery |
| Memory leak monitoring | ✅ No leak trend detected |
| Heap stability | ✅ 4.1–4.2 MB steady state |
| Compiler warnings | ✅ 0 warnings |
| Code quality | ✅ All files < 250 lines |

---

## 🚀 Quick Start | 快速开始

```bash
# 1. Source ESP-IDF
. $HOME/esp/esp-idf/export.sh

# 2. Configure WiFi (credentials never stored in repo)
bash tools/provision-wifi.sh

# 3. Build & Flash
idf.py build
idf.py -p /dev/cu.wchusbserial110 flash monitor

# 4. Open browser → http://<device-ip>/
```

See [README.md](README.md) for detailed instructions.

---

## 📸 Screenshots | 界面截图

| MJPEG Stream + HUD | WebSocket Stream + Controls |
|:---:|:---:|
| ![MJPEG](docs/images/mjpeg-stream.png) | ![WebSocket](docs/images/ws-stream.png) |

---

## 📅 Development Timeline | 开发时间线

| Day | Milestone | Deliverable |
|-----|-----------|-------------|
| 1 | M0 Scaffold | ESP-IDF project + WiFi manager |
| 3 | M1 TCP Stream | MJPEG over HTTP |
| 4 | M3 LED Control | GPIO33 web control |
| 5 | M2 HUD Overlay | FPS + temperature overlay |
| 8 | M4 WebSocket | WS video + control + heartbeat |
| 11 | M5 Stability | Memory + stress + reconnect tests |
| 12 | Release Prep | Bilingual docs + screenshots |
| 13 | **v1.0.0** | **🎉 First official release** |

Every code change went through: **build → flash → serial verify → browser verify** on real hardware.

---

## 🛠️ Tools Included | 附带工具

- `tools/provision-wifi.sh` — WiFi credential injection
- `tools/heap_monitor.py` — Real-time heap trend monitor
- `tools/multi_client_test.py` — Multi-client stress test
- `tools/wifi_reconnect_test.py` — WiFi reconnect test
- `tools/browser_verify.py` — Browser automation verification
- `tools/take_screenshots.py` — Documentation screenshot capture
