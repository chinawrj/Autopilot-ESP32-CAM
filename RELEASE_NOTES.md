# v1.2.0 — mDNS Discovery + Camera Settings | mDNS 发现 + 摄像头设置

## Autopilot ESP32-CAM v1.2.0

New features: mDNS local network discovery and real-time camera parameter adjustment.

新功能：mDNS 局域网发现和实时摄像头参数调节。

---

## ✨ New Features | 新功能

### mDNS Local Discovery | mDNS 局域网发现

Access your ESP32-CAM via `http://espcam.local/` — no need to remember IP addresses.

通过 `http://espcam.local/` 访问你的 ESP32-CAM — 无需记住 IP 地址。

- Uses `espressif/mdns` managed component v1.11.0
- Socket-based networking for better compatibility
- Hostname: `espcam`, advertises `_http._tcp` service on port 80
- Note: Requires router to allow WiFi multicast between clients

### Camera Settings API | 摄像头设置 API

Real-time camera parameter adjustment via REST API and dashboard UI.

通过 REST API 和仪表盘界面实时调节摄像头参数。

- **`GET /api/camera`** — Returns current camera settings as JSON
- **`POST /api/camera`** — Update settings with partial JSON (only send changed fields)
- Parameters: `brightness`, `contrast`, `saturation`, `sharpness` (-2 to 2)
- Toggles: `hmirror`, `vflip` (true/false)
- Read-only: `quality`, `framesize`

### Camera Settings UI Panel | 摄像头设置面板

- Collapsible panel on the unified dashboard
- Range sliders for image adjustments
- Checkboxes for mirror/flip
- Changes applied instantly to camera sensor

---

## 📊 Verified | 已验证

- ✅ Build: 0 warnings
- ✅ Flash + Serial: v1.2.0 boot, mDNS started, all services OK
- ✅ API: `/api/status` (version 1.2.0), `/api/camera` GET + POST
- ✅ Browser: Dashboard with Camera Settings panel visible
- ✅ mDNS responder: Unicast query returns correct IP

---

## 📊 Performance | 性能指标

| Metric | Value |
|--------|-------|
| MJPEG FPS | ~10 fps (VGA) |
| WebSocket FPS | ~10 fps (VGA) |
| Multi-client | 3 WS + 1 MJPEG, 0 errors (300s) |
| Free Heap | ~4.2 MB |
| Firmware Size | ~1.0 MB |
| Partition Layout | Dual OTA (3MB × 2) + 1.97MB SPIFFS |
| Total C Code | ~1300 lines across 9 source files |
| QA Coverage | 130/130 tests (80 API + 50 browser) |

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
