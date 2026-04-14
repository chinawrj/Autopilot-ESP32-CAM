# v2.0.0 — Production-Ready Stable Release | 生产就绪稳定版

## Autopilot ESP32-CAM v2.0.0

The first production-ready release. 33 days of daily hardware-verified iterations
delivering a complete autonomous camera web server with monitoring, security, and
comprehensive test coverage.

首个生产就绪版本。33 天每日硬件验证迭代，交付完整的自动驾驶摄像头 Web 服务器，
具备监控、安全和全面的测试覆盖。

---

## 🆕 What's New Since v1.3.0 | 新增内容

### Reliability | 可靠性

- **Task Watchdog (30s)** — Hardware WDT auto-reboots on task hang
- **Heap Monitoring** — Periodic integrity checks + low-memory warnings
- **Error Audit** — All error paths hardened, no silent failures

### Security | 安全

- **Path Traversal Protection** — SD card paths validated against `../` attacks
- **HTTP Security Headers** — nosniff, SAMEORIGIN, no-store on all responses
- **Rate Limiting** — Token bucket algorithm, 429 responses with Retry-After

### Features | 功能

- **System Diagnostics** — `/api/system/info` with health status, chip info, memory, uptime
- **Enhanced Dashboard** — Real-time panels: chip, IDF version, PSRAM, tasks
- **App.js Extraction** — Cacheable frontend JS, deferred MJPEG loading
- **API Documentation** — Complete REST API reference at `docs/API.md`

### Testing | 测试

- **50 Unit Tests** — 5 suites covering all modules (host-based, no hardware needed)
- **39 Browser Tests** — Patchright integration tests covering all endpoints + security

---

## 📊 Verified | 已验证

- ✅ Build: 0 warnings, binary ~1.29 MB (59% partition free)
- ✅ Unit tests: 50/50 via `ctest` (5 suites, host-based)
- ✅ Browser tests: 39/39 via Patchright (hardware-verified)
- ✅ Flash + Serial: All services start, watchdog active, heap healthy
- ✅ All 20 REST endpoints responding correctly
- ✅ Dual video streams: MJPEG (:8081) + WebSocket (:80)

---

## 📊 Performance | 性能指标

| Metric | Value |
|--------|-------|
| Firmware Size | ~1.29 MB |
| Free Heap | ~137 KB internal, ~4 MB total |
| PSRAM Free | ~3943 KB / 4096 KB |
| MJPEG FPS | ~10 fps (VGA) |
| WebSocket FPS | ~10 fps (VGA) |
| Boot Time | < 3 seconds |
| WiFi Reconnect | ~2s recovery |
| Unit Tests | 50/50 (5 suites) |
| Integration Tests | 39/39 (Patchright) |
| Source Files | 14 C files, ~2000 lines |

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

# 5. Run tests
cd test && mkdir -p build && cd build && cmake .. && make && ctest  # unit tests
python3 test/integration/test_browser.py --device <IP>               # browser tests
```

See [README.md](README.md) and [docs/API.md](docs/API.md) for full documentation.

---

## 📅 Development Timeline | 开发时间线

| Day | Milestone | Deliverable |
|-----|-----------|-------------|
| 1 | M0 Scaffold | ESP-IDF project + WiFi manager |
| 13 | **v1.0.0** | 🎉 First release |
| 18 | **v1.1.0** | OTA + Snapshot + Dashboard |
| 21 | **v1.2.0** | mDNS + Camera Settings |
| 23 | **v1.3.0** | SD Card + Unit Tests |
| 27 | **v1.4.0** | Security + Diagnostics |
| 33 | **v2.0.0** | 🏆 **Production-ready stable release** |

Every code change: **build → flash → serial verify → browser verify** on real hardware.
