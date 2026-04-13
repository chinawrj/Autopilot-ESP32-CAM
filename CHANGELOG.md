# Changelog

All notable changes to Autopilot ESP32-CAM are documented in this file.

## [v1.2.0] — 2026-04-13

### ✨ New Features

- **mDNS local discovery** — Access the device via `http://espcam.local/` without remembering the IP address
  - Uses `espressif/mdns` managed component v1.11.0
  - Socket-based networking mode for better compatibility
  - Hostname: `espcam`, service: `_http._tcp` on port 80
  - Note: Multicast discovery requires router to allow WiFi client-to-client multicast

- **Camera Settings API** — Adjust camera parameters in real-time via `GET/POST /api/camera`
  - Brightness, contrast, saturation, sharpness (-2 to 2)
  - Horizontal mirror, vertical flip (on/off)
  - Quality and framesize (read via GET)
  - Supports partial JSON updates (send only changed fields)

- **Camera Settings UI panel** — Collapsible panel on the dashboard
  - Range sliders for brightness, contrast, saturation, sharpness
  - Checkboxes for horizontal mirror and vertical flip
  - Settings applied instantly to the camera sensor

### 🔧 Technical

- mDNS initialized after WiFi with manual `mdns_netif_action()` to handle GOT_IP event race
- `max_uri_handlers` bumped from 14 to 16 to accommodate new routes
- Added `esp_netif.h` include for netif handle lookup

---

## [v1.1.1] — 2026-04-14

### 🐛 Bug Fixes

- **MJPEG stream port 81 → 8081** — Changed from port 81 to 8081 to avoid transparent HTTP proxy interception on common low ports (discovered during quality audit)

### 📝 Documentation

- **README overhaul** — Updated both README.md and README_CN.md to reflect v1.1.x architecture:
  - Architecture diagram: single server → dual server (:80 API + :8081 MJPEG)
  - Features table: added OTA, Snapshot, Unified Dashboard, System Info
  - API Reference: added `/api/snapshot`, `/api/ota`, `/api/ota/status`
  - Project structure: added `stream_server.c/h`, `ota_update.c/h`, new test tools
  - Performance metrics: updated code stats (~1300 lines / 9 source files)
  - Partition layout: updated to dual OTA (3MB × 2) + 1.97MB SPIFFS
  - Milestone timeline: added v1.1.0, quality audit, v1.1.1

### 🧪 Testing

- ✅ Quality audit: 130/130 tests passed (80 API + 50 browser)
- `tools/quality_audit.py` — 80-item API test suite
- `tools/browser_qa.py` — 50-item Patchright browser UI test suite

---

## [v1.1.0] — 2026-04-13

### ✨ New Features

- **OTA Firmware Update** — Over-the-air upgrade via `POST /api/ota` with HTTP URL
  - Dual OTA partition layout (ota_0 + ota_1, 3MB each)
  - Progress tracking via `GET /api/ota/status`
  - Automatic rollback protection
  - Web UI: OTA panel with URL input + progress bar + version display
- **Snapshot API** — `GET /api/snapshot` returns a single JPEG frame for capture/download
- **Unified Dashboard** — Single-page UI with tab switching (TCP / WebSocket streams)
  - All controls on one page: stream, snapshot, LED, OTA, system info
- **System Info Panel** — Firmware version, uptime, free heap, min heap, WiFi RSSI
- **WiFi RSSI Monitoring** — `GET /api/status` now includes `rssi`, `uptime`, `wifi_connected`

### 🏗️ Architecture Changes

- **Dual HTTP Server** — Port 80 (API + pages + WebSocket) / Port 8081 (MJPEG stream)
  - Fixes MJPEG stream blocking API responses on single-server architecture
  - `stream_server.c` extracted as independent MJPEG server module

### 🐛 Bug Fixes

- **Camera I2C recovery after OTA reboot** — `esp_camera_deinit()` before `esp_restart()` to properly release I2C bus; prevents OV2640 probe timeout after software reset
- **Camera PWDN power-cycle timing** — Extended to 500ms + I2C bus recovery (clock pulse bit-banging) for reliable camera init after any restart path
- **MJPEG stream port** — Changed from 81 to 8081 to avoid transparent HTTP proxy interception on common low ports

### 📊 Performance

| Metric | Value |
|--------|-------|
| MJPEG FPS | ~10 fps (VGA) |
| WebSocket FPS | ~10 fps (VGA) |
| Multi-client | 3 WS + 1 MJPEG, 0 errors (300s) |
| Free Heap | ~4.2 MB |
| WiFi Reconnect | ~2s recovery |
| OTA Download | ~15s for 1MB firmware |

### 🧪 Testing

- ✅ Full endpoint regression (10 API endpoints + dual video streams)
- ✅ Browser automation test (Patchright, 17/18 checks pass)
- ✅ OTA end-to-end test (download → flash → reboot → camera OK → HTTP OK)
- ✅ Multi-client stress test (3 WS + 1 MJPEG, 300s, 0 errors)
- ✅ WiFi disconnect/reconnect (3/3 rounds, ~2s recovery)
- ✅ Heap stability (no leak trend, ±50KB drift over 300s)
- ✅ All code < 250 lines/file, 0 compiler warnings

### 🔧 New Tools

- `tools/regression_test.py` — Automated browser regression test suite
- `tools/stability_test.sh` — Multi-client stability test script
- `tools/ota_e2e_test.py` — OTA end-to-end test with HTTP server

---

## [v1.0.0] — 2026-04-09

### 🎉 Initial Release — Complete Camera Web Server

Complete camera web server for YD-ESP32-CAM (ESP32-WROVER-E-N8R8).

### Features

- **TCP Video Streaming** — MJPEG over HTTP at `/stream/tcp`, browser-native playback
- **WebSocket Video Streaming** — Binary JPEG push at `/ws/stream` with Canvas rendering
  - Up to 4 concurrent WebSocket clients
  - Dynamic quality control (Q10–Q50)
  - Dynamic resolution control (QVGA / VGA / SVGA / XGA)
  - 5-second heartbeat with FPS, client count, and heap stats
- **Real-time HUD Overlay** — FPS counter + virtual temperature sensor (25°C ±3°C)
- **LED Remote Control** — Web button and REST API (`POST /api/led`) for GPIO33
- **Status API** — `GET /api/status` returns JSON with fps, temperature, LED state, heap memory
- **WiFi Management** — STA mode with exponential backoff auto-reconnect (1s → 10s)
- **Security** — WiFi credentials never stored in repository; injected via `tools/provision-wifi.sh`
- **Bilingual Documentation** — English (`README.md`) + Chinese (`README_CN.md`) with screenshots

### Hardware Support

- YD-ESP32-CAM (VCC-GND Studio)
- ESP32-WROVER-E-N8R8 (8MB Flash + 8MB PSRAM)
- OV2640 Camera (AI-Thinker compatible pinout)

### Performance

| Metric | Value |
|--------|-------|
| MJPEG FPS | ~10 fps (VGA) |
| WebSocket FPS | ~10 fps (VGA) |
| Multi-client | 2 WS + 1 MJPEG, 0 errors |
| Free Heap | ~4.1 MB |
| Firmware Size | ~1034 KB |

### Testing

- ✅ Multi-client stress test (2 WS + 1 MJPEG, 60s, 0 errors)
- ✅ WiFi disconnect/reconnect (4/4 successful, ~2s recovery)
- ✅ Memory leak monitoring (no leak trend detected)
- ✅ Heap stability (4.1–4.2 MB steady state)
- ✅ All code < 250 lines/file, 0 compiler warnings

### Development Timeline

| Day | Milestone | Key Deliverable |
|-----|-----------|----------------|
| 1 | M0 Scaffold | ESP-IDF project + WiFi manager |
| 3 | M1 TCP Stream | MJPEG over HTTP |
| 4 | M3 LED Control | GPIO33 web control |
| 5 | M2 HUD | FPS + temperature overlay |
| 8 | M4 WebSocket | WS video + control + heartbeat |
| 11 | M5 Stability | Memory + stress + reconnect tests |
| 12 | Release Prep | Bilingual docs + screenshots |

### Tools Included

- `tools/provision-wifi.sh` — Secure WiFi credential injection
- `tools/heap_monitor.py` — Real-time heap memory trend monitor
- `tools/multi_client_test.py` — Multi-client concurrent stress test
- `tools/wifi_reconnect_test.py` — WiFi disconnect/reconnect test
- `tools/browser_verify.py` — Automated browser verification
- `tools/take_screenshots.py` — Documentation screenshot capture
