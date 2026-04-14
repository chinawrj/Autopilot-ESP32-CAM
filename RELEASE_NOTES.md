# v1.3.0 — SD Card Storage + Unit Tests | SD 卡存储 + 单元测试

## Autopilot ESP32-CAM v1.3.0

SD card photo capture and file management, code refactoring with extracted components, and host-based unit test framework.

SD 卡拍照存储和文件管理，代码重构提取独立组件，以及基于主机的单元测试框架。

---

## ✨ New Features | 新功能

### SD Card Storage | SD 卡存储

Micro SD card support for capturing and managing JPEG snapshots.

支持 Micro SD 卡，用于拍摄和管理 JPEG 快照。

- **1-bit SDMMC mode** — Uses GPIO2/14/15, avoids GPIO4 (Flash LED) and GPIO12 (MTDI) conflicts
- **Auto-detect** — Mounts on boot if card is present; graceful degradation when no card inserted
- **5 REST APIs**: `GET /api/sd/status`, `GET /api/sd/list`, `POST /api/sd/capture`, `GET /api/sd/file/*`, `POST /api/sd/delete`
- **Web UI panel** — Collapsible "SD Card Storage" panel with capture button, file browser, download links, and delete
- **Long filenames** — FATFS LFN support enabled (`CONFIG_FATFS_LFN_HEAP=y`)

### Code Quality Improvements | 代码质量改进

- **FPS Counter Component** — Extracted `components/fps_counter/` with pure struct API
- **JSON Helper** — `http_helpers.c/h` eliminates 5 duplicate JSON response patterns
- **20 Unit Tests** — Host-based Unity framework, runs on Mac/Linux without hardware
  - fps_counter (7), virtual_sensor (6), led_controller (7) — all passing

---

## 📊 Verified | 已验证

- ✅ Build: 0 warnings, binary 1.2 MB (59% partition free)
- ✅ Unit tests: 20/20 via `ctest` (host-based)
- ✅ Flash + Serial: SD card mount, all services OK
- ✅ Browser: SD Card panel with capture/list/delete (Patchright 8/8)

---

## 📊 Performance | 性能指标

| Metric | Value |
|--------|-------|
| MJPEG FPS | ~10 fps (VGA) |
| WebSocket FPS | ~10 fps (VGA) |
| Multi-client | 3 WS + 1 MJPEG, 0 errors (300s) |
| Free Heap | ~4.2 MB |
| Firmware Size | ~1.2 MB |
| Partition Layout | Dual OTA (3MB × 2) + 1.97MB SPIFFS |
| Total C Code | ~1600 lines across 13 source files |
| Unit Tests | 20/20 (3 suites) |

---

## 🔧 Hardware | 硬件

| Parameter | Value |
|-----------|-------|
| Board | YD-ESP32-CAM (VCC-GND Studio) |
| Module | ESP32-WROVER-E-N8R8 (8MB Flash + 8MB PSRAM) |
| SoC | ESP32-D0WD-V3 (Dual-core Xtensa LX6, 240MHz) |
| Camera | OV2640 (AI-Thinker compatible pinout) |
| Onboard LED | GPIO33 |
| SD Card | 1-bit SDMMC (GPIO2/14/15) |

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

# 5. Run unit tests (no hardware needed)
cd test && mkdir -p build && cd build && cmake .. && make && ctest
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
| 13 | **v1.0.0** | **🎉 First official release** |
| 18 | **v1.1.0** | OTA + Snapshot + Unified Dashboard |
| 21 | **v1.2.0** | mDNS + Camera Settings |
| 22 | Refactoring | FPS component + JSON helper + 20 unit tests |
| 23 | **v1.3.0** | **SD Card Storage + Unit Tests** |

Every code change went through: **build → flash → serial verify → browser verify** on real hardware.

---

## 🛠️ Tools Included | 附带工具

- `tools/provision-wifi.sh` — WiFi credential injection
- `tools/heap_monitor.py` — Real-time heap trend monitor
- `tools/multi_client_test.py` — Multi-client stress test
- `tools/wifi_reconnect_test.py` — WiFi reconnect test
- `tools/browser_verify.py` — Browser automation verification
- `tools/take_screenshots.py` — Release screenshot capture
