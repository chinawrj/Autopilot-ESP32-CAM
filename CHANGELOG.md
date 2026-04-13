# Changelog

All notable changes to Autopilot ESP32-CAM are documented in this file.

## [v1.0.0] — 2026-04-15 (Planned Release)

### 🎉 Initial Release

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
