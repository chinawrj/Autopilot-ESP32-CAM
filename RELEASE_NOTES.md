# v1.1.1 — Patch Release | 补丁发布

## Autopilot ESP32-CAM v1.1.1

Patch release with documentation refresh and port fix. All documentation now accurately reflects the v1.1.x dual-server architecture.

补丁发布：文档全面刷新 + 端口修复。所有文档现已准确反映 v1.1.x 双服务器架构。

---

## 🐛 Bug Fixes | 修复

- **MJPEG stream port 81 → 8081** — Avoids transparent HTTP proxy interception on low ports (discovered during Day 19 quality audit, 130/130 tests passed)

## 📝 Documentation | 文档

- **README overhaul** — Both EN/CN README updated to match v1.1.x reality:
  - Architecture: dual HTTP server (:80 API + :8081 MJPEG stream)
  - Features: +OTA firmware update, +Snapshot API, +Unified Dashboard, +System Info
  - API Reference: +`/api/snapshot`, +`/api/ota`, +`/api/ota/status`
  - Project structure: +`stream_server.c/h`, +`ota_update.c/h`, +QA test tools
  - Performance: ~1300 lines / 9 source files, dual OTA partition layout
  - Milestone timeline: added v1.1.0 through v1.1.1

## 🧪 QA Tools | 测试工具

- `tools/quality_audit.py` — 80-item API test suite (curl-based)
- `tools/browser_qa.py` — 50-item browser UI test suite (Patchright Chrome)

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
