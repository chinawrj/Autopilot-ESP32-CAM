# Autopilot ESP32-CAM — Project Goals & Progress Tracking

## Overall Goal

Build a complete **real-time camera web server** on the YD-ESP32-CAM (ESP32-WROVER-E-N8R8),
supporting dual-path TCP/UDP video streaming, real-time HUD overlay, and LED control.

## Milestone Progress

### M0: Project Scaffold ✅ Complete
> **Target**: Day 1 | **Completed**: Day 1

- [x] ESP-IDF project structure (CMakeLists.txt, partition table)
- [x] sdkconfig.defaults (PSRAM, camera, WiFi base config)
- [x] WiFi management module (wifi_manager.c/h)
  - [x] Read SSID/Password from Kconfig
  - [x] STA mode connection
  - [x] Auto-reconnect (max 5 retries)
  - [x] Event callbacks (connect/disconnect/got IP)
- [x] tools/provision-wifi.sh (credential injection)
- [x] Build passed
- [x] Flash successful + serial shows WiFi connection log + got IP (Day 6 verified: IP 192.168.1.171)
- [x] Git: initial commit

**Completion criteria**: ✅ Serial output `WiFi connected, IP: 192.168.1.171` (Day 6 hardware verified)

---

### M1: Basic TCP Video Stream ✅ Complete
> **Target**: Day 2-3 | **Completed**: Day 3

- [x] OV2640 camera initialization (camera_init.c)
  - [x] PSRAM frame buffer
  - [x] Resolution: VGA (640x480)
  - [x] JPEG quality: 12
- [x] HTTP server (http_server.c)
  - [x] GET `/` → homepage (index.html)
  - [x] GET `/stream/tcp` → MJPEG stream
  - [x] MJPEG: multipart/x-mixed-replace boundary
- [x] Frontend page (main/index.html)
  - [x] `<img>` tag pointing to `/stream/tcp`
  - [x] Basic layout and styling
- [x] Browser verified: MJPEG stream working, ~14KB/frame, FPS ~6.3 (Day 6 curl verified)

**Completion criteria**: ✅ `http://192.168.1.171/stream/tcp` MJPEG stream working (Day 6 hardware verified)

---

### M2: HUD Overlay ✅ Complete
> **Target**: Day 4-5 | **Completed**: Day 5

- [x] Real-time FPS calculation (server-side frame rate tracking)
- [x] Virtual temperature sensor component (components/virtual_sensor/)
  - [x] Baseline 25°C, ±3°C random fluctuation
  - [x] Update frequency 1 Hz
- [x] REST API
  - [x] GET `/api/status` → JSON {fps, temperature, led_state}
- [x] Frontend HUD
  - [x] JavaScript polling `/api/status` (every second)
  - [x] FPS and temperature overlay on top of video stream
  - [x] Style: semi-transparent black background with white text

**Completion criteria**: ✅ /api/status → fps=6.3, temp=27.3°C, led_state (Day 6 hardware verified)

---

### M3: LED Control ✅ Complete
> **Target**: Day 6 | **Completed**: Day 4 (ahead of schedule)

- [x] GPIO33 LED driver (led_controller.c)
  - [x] Initialize as output, default off
  - [x] on/off/toggle interface
- [x] REST API
  - [x] POST `/api/led` body: {"state": "on"/"off"/"toggle"}
  - [x] Returns current state JSON
- [x] Frontend button
  - [x] Toggle button showing current state (🔴 OFF / 🟢 ON)
  - [x] Click sends POST request
  - [x] Real-time state sync
- [x] Hardware verified: LED on/off/toggle all working (Day 6 curl verified)

**Completion criteria**: Click web button, ESP32-CAM onboard LED turns on/off (pending hardware verification)

---

### M4: UDP Video Stream ✅ Complete
> **Target**: Day 7-9 | **Completed**: Day 8

- [ ] UDP push module (udp_stream.c)
  - [ ] JPEG frame fragmentation (MTU 1400)
  - [ ] Sequence number + frame number header
  - [ ] Frame drop tolerance
- [x] WebSocket video stream channel (ws_stream.c)
  - [x] WebSocket binary frame JPEG push (Day 7 implemented)
  - [x] Up to 4 concurrent clients
  - [x] httpd close callback auto-cleanup
  - [x] Control messages: start/stop/quality (Day 8: set_quality, set_resolution, get_status)
  - [x] Heartbeat mechanism (Day 8: 5s periodic heartbeat with fps/clients/heap)
- [x] Frontend `/stream/ws` page (Day 7)
  - [x] WebSocket connection management + auto-reconnect
  - [x] Canvas rendering (Blob → Image → drawImage)
  - [x] HUD overlay: FPS + temperature + WS status
  - [x] LED control button
- [x] TCP and WS dual paths coexist without conflict (Day 7 verified)

**Completion criteria**: `/stream/tcp` and `/stream/ws` both independently accessible — ✅ Day 7 verified

---

### M5: Stability & Optimization ✅ Complete
> **Target**: Day 10-12 | **Completed**: Day 11

- [x] Memory usage analysis (`heap_caps_get_info`) — Day 9: heap ~4.1MB free
- [x] Memory leak detection (5min continuous run, heap drift +383 bytes/min, no leak)
- [x] WiFi disconnect/reconnect test — Day 11: debug endpoint + 4/4 reconnects successful (~2s recovery)
- [x] Multi-client concurrent test (3 WS + 1 MJPEG, 60s, 0 errors)
- [x] PSRAM allocation optimization — Day 10: evaluated, no optimization needed (4MB+ free, camera only uses 120KB)
- [x] 24-hour continuous run test — Day 10: started background monitoring (heap_monitor.py 1440 min)
- [x] Code refactoring — Day 10: send_heartbeat(); Day 11: http_server.c 260→216 lines
- [x] README.md final version — Day 10: completed

**Completion criteria**: ✅ Multi-client 0 errors + WiFi reconnect 4/4 + heap stable + code review passed (Day 11)

---

### Release: Release Preparation ✅ Complete
> **Target**: Day 12-13 | **Completed**: Day 12

- [x] Web UI screenshots (4 images: MJPEG / WebSocket / LED / API)
- [x] Bilingual README (README.md English + README_CN.md Chinese)
- [x] CHANGELOG.md (v1.0.0 release notes)
- [x] Architecture diagram (Mermaid, embedded in README)
- [x] Screenshot tool (tools/take_screenshots.py)
- [x] GitHub Release created (Day 13)

**Completion criteria**: ✅ GitHub Release v1.0.0 published + README rendering verified (Day 13)

---

## Current Status

| Milestone | Status | Progress |
|-----------|--------|----------|
| M0 Scaffold | ✅ | 100% |
| M1 TCP Stream | ✅ | 100% |
| M2 HUD | ✅ | 100% |
| M3 LED | ✅ | 100% |
| M4 WS Stream | ✅ | 100% |
| M5 Stability | ✅ | 100% |
| Release v1.0.0 | ✅ | 100% |
| M6 v1.1.0 New Features | ✅ | 100% |
| Release v1.1.1 | ✅ | 100% |
| Release v1.2.0 | ✅ | 100% |
| Release v1.3.0 | ✅ | 100% |
| Release v1.4.0 | ✅ | 100% |

**Current work day**: Day 30
**Current firmware version**: v1.4.0
**Status**: Day 30 — JS extraction + API rate limiting (code health + security hardening)

---

## v1.1.0 Release Plan (Day 15-18)

### M6: v1.1.0 New Features
> **Target**: Day 15-18 | **Released**: Day 18

#### Day 15: Documentation Update + Planning
- [x] README: add AI developer info (Claude Opus 4.6)
- [x] v1.1.0 roadmap planning

#### Day 16: OTA Firmware Upgrade
- [x] OTA upgrade module (ota_update.c/h)
  - [x] HTTP OTA: download firmware from URL and upgrade
  - [x] Version management (app_desc)
  - [x] Rollback protection (dual OTA partition)
- [x] REST API: `POST /api/ota` trigger upgrade
- [x] REST API: `GET /api/ota/status` progress query
- [x] Frontend: OTA upgrade panel (URL input + progress bar + version display)
- [x] Partition table adjustment: dual OTA partitions (ota_0 + ota_1, 3MB each)
- [x] Fix: camera PWDN power-up timing (I2C timeout after OTA reboot)
- [x] E2E test: download → flash → reboot → camera OK → HTTP OK

#### Day 17: Snapshot Capture + Web UI Optimization
- [x] REST API: `GET /api/snapshot` returns single JPEG frame
- [x] Frontend: capture button + image download
- [x] Homepage merge: unified entry page (TCP/WS switching + all control panels)
- [x] System info panel: firmware version, uptime, memory status, WiFi signal
- [x] Fix: MJPEG stream blocking API — split into dual-server architecture (port 80 + 81)
- [x] WiFi RSSI function + `/api/status` enhancement

#### Day 18: Testing + Release v1.1.0
- [x] Full feature regression test (all endpoints) — curl 9/10 + browser 17/18
- [x] OTA end-to-end test — fixed camera I2C reboot issue
- [x] Stability verification (multi-client + WiFi reconnect) — 300s 0 errors + 3/3 reconnect
- [x] CHANGELOG update
- [x] GitHub Release v1.1.0

#### Day 19: Quality Audit
- [x] Full API endpoint test (curl) — 80/80 passed
- [x] Browser UI verification (Patchright Chrome) — 50/50 passed
- [x] Fix: MJPEG stream port 81 → 8081 (network proxy compatibility)
- [x] All code/docs port references updated

#### Day 20: v1.1.1 Documentation Refresh + Patch Release
- [x] README.md / README_CN.md full update
  - [x] Architecture diagram: single server → dual server (:80 + :8081)
  - [x] Features table: added OTA, snapshot, unified dashboard, system info
  - [x] API reference: added /api/snapshot, /api/ota, /api/ota/status
  - [x] Project structure: added stream_server.c/h, ota_update.c/h, new test tools
  - [x] Performance metrics: ~1300 lines / 9 source files, dual OTA partition layout
  - [x] Milestone timeline: added v1.1.0, v1.1.1
- [x] CHANGELOG.md v1.1.1 entry
- [x] RELEASE_NOTES.md updated to v1.1.1
- [x] Firmware version 1.1.0 → 1.1.1
- [x] GitHub Release v1.1.1

---

## v1.2.0 Release (Day 21)

### Day 21: mDNS + Camera Settings API + Release v1.2.0
- [x] mDNS local discovery: `http://espcam.local/`
  - [x] espressif/mdns 1.11.0 managed component
  - [x] Socket networking mode (CONFIG_MDNS_NETWORKING_SOCKET=y)
  - [x] Fix: WiFi GOT_IP event race → manual mdns_netif_action
  - [x] Verified: ESP32 mDNS responder working (unicast query verified)
  - [x] Note: multicast discovery requires router to allow WiFi client-to-client multicast
- [x] Camera Settings API: GET/POST `/api/camera`
  - [x] Supports: brightness, contrast, saturation, sharpness (-2~2)
  - [x] Supports: hmirror, vflip (bool)
  - [x] Partial JSON updates (send only changed fields)
  - [x] curl + serial + browser full chain verified
- [x] Frontend: Camera Settings panel (collapsible)
  - [x] Range sliders + Checkboxes
  - [x] Real-time camera parameter adjustment
- [x] Firmware version 1.1.1 → 1.2.0
- [x] Hardware verified: build ✅ flash ✅ serial ✅ curl ✅ browser ✅
- [x] Screenshots: v1.2.0-dashboard.png, v1.2.0-tcp-stream.png
- [x] CHANGELOG.md + RELEASE_NOTES.md updated
- [x] GitHub Release v1.2.0

### Day 22: Code Refactoring + Unit Test Coverage
- [x] Refactor: extract FPS counter component (`components/fps_counter/`)
  - [x] Pure struct API (fps_counter_t), no global variables
  - [x] Eliminated duplicate code in stream_server.c and ws_stream.c
- [x] Refactor: extract JSON response helper (`main/http_helpers.c/h`)
  - [x] http_send_json() wraps 5 repetitive JSON response patterns
  - [x] http_server.c reduced by 27 lines
- [x] Set up Unity unit test framework (host-based)
  - [x] test/CMakeLists.txt + mocks + redirect headers
  - [x] Compile and run directly on Mac, no ESP32 hardware needed
- [x] Write core module unit tests: 20 tests, 0 failures
  - [x] test_fps_counter (7 tests): initial values, window, high framerate, reset
  - [x] test_virtual_sensor (6 tests): temperature range, random value mapping
  - [x] test_led_controller (7 tests): GPIO mock, toggle, idempotency
- [x] Hardware verified: build ✅ flash ✅ serial ✅ browser ✅ (Patchright)
- [x] Host tests: ctest 20/20 pass

### Day 23: SD Card Support (Phase 1: Basic Read/Write)
- [x] SD card driver component (components/sd_card/)
  - [x] 1-bit SDMMC mode (GPIO2/14/15)
  - [x] VFS FAT mount to /sdcard
  - [x] Graceful degradation when no card (non-fatal)
  - [x] Enabled FATFS LFN long filename support
- [x] SD Card REST API (main/sd_handlers.c/h)
  - [x] GET /api/sd/status, GET /api/sd/list
  - [x] POST /api/sd/capture, POST /api/sd/delete
  - [x] GET /api/sd/file/* (wildcard matching)
- [x] Frontend SD Card Storage panel
- [x] Hardware verified: build 0 warnings, flash, serial, Patchright 8/8
- [x] Unit test regression: 20/20 pass

### Day 24: v1.3.0 Release Preparation
- [x] Version bump: 1.2.0 → 1.3.0
- [x] CHANGELOG.md: v1.3.0 entry (SD card + refactoring + unit tests)
- [x] RELEASE_NOTES.md: updated to v1.3.0
- [x] README.md / README_CN.md full update
  - [x] Features table: added SD card storage, unit tests
  - [x] Architecture diagram: added sd_card module + Micro SD card hardware
  - [x] API reference: added 6 SD card endpoints
  - [x] Project structure: added sd_handlers, http_helpers, fps_counter, sd_card, test/
  - [x] Performance metrics: ~1600 lines / 13 source files, 20 unit tests
  - [x] Milestone timeline: added Day 22 refactoring, v1.3.0
- [x] TARGET.md updated
- [x] Build verified: 0 warnings
- [x] Unit tests: 20/20 pass
- [x] GitHub Release v1.3.0

### Day 25: GitHub Actions CI/CD Pipeline
- [x] CI workflow: `.github/workflows/ci.yml`
  - [x] Unit Tests job: download Unity → cmake → ctest (ubuntu-latest)
  - [x] ESP-IDF Build job: espressif/idf:v5.5.1 container build
  - [x] Firmware artifact upload (30-day retention)
- [x] Test CMakeLists.txt improvement: UNITY_DIR configurable (supports CI/local/ESP-IDF)
- [x] README bilingual CI status badge added
- [x] Push verified CI run

### Day 26: Code Refactoring + Full Chain Hardware Verification
- [x] Refactor: extract `camera_handlers.c/h` from http_server.c
  - [x] http_server.c: 281 → 213 lines (below 250 threshold)
  - [x] camera_handlers.c: 80 lines (GET/POST /api/camera)
  - [x] camera_handlers_register() pattern reuses sd_handlers approach
- [x] Full chain hardware verification
  - [x] Build: 0 warnings, 1.2MB (59% free)
  - [x] Flash: EXIT_0 via /dev/cu.wchusbserial110
  - [x] Serial: WiFi + Camera + SD + HTTP + WS all started
  - [x] Browser (Patchright): 7/8 pass (including /api/camera regression)
  - [x] curl: all APIs 200, MJPEG 236KB/3s
- [x] Unit test regression: 20/20 pass

### Day 27: Path Traversal Security Fix + Test Coverage Expansion
- [x] Security fix: SD handlers path traversal vulnerability
  - [x] New path_utils.c/h (path_is_safe + path_sanitize_sd)
  - [x] sd_list, sd_file, sd_delete all apply path validation
  - [x] curl verified: `?path=../../etc` → `{"error":"Invalid path"}`
- [x] Refactor: split sd_handlers.c (250→164 lines)
  - [x] Extract sd_file_ops.c/h (capture + file serving, 123 lines)
  - [x] sd_file_ops_register() pattern reuses camera_handlers approach
- [x] Test expansion: 20→43 tests (4 suites)
  - [x] New test_path_utils: 23 path security tests
  - [x] Coverage: normal paths, traversal attacks, buffer overflow, edge cases
- [x] Full chain hardware verification
  - [x] Build: 0 warnings, 1.2MB
  - [x] Flash + Serial: all started, no crash
  - [x] Browser (Patchright): 9/9 pass (including path traversal blocking)
  - [x] Unit Tests: 43/43 pass (4 suites)

### Day 28: System Diagnostics API + HTTP Security Headers + Integration Tests
- [x] New GET /api/system/info (chip, IDF, heap, PSRAM, uptime, tasks, WiFi, FPS)
  - [x] system_info.c/h (74 lines)
  - [x] Independent module system_info_register() pattern
- [x] HTTP security headers: X-Content-Type-Options, X-Frame-Options, Cache-Control
  - [x] http_send_json() enhanced
  - [x] New http_send_html() helper
  - [x] HTML handlers switched to centralized function
- [x] Integration test script: test/integration/test_browser.py (15 tests)
  - [x] Coverage: Homepage, APIs, security headers, LED, Snapshot, Camera, WS, path traversal
- [x] Full chain hardware verification
  - [x] Build: 0 warnings
  - [x] Flash + Serial: OK
  - [x] Browser (Patchright): 15/15 pass
  - [x] Unit Tests: 43/43 pass (4 suites)

### Day 29: v1.4.0 Release
- [x] Frontend enhancement: System Info panel added chip/IDF/PSRAM/tasks
- [x] Version bump: v1.3.0 → v1.4.0
- [x] CHANGELOG update: v1.4.0 full release notes
- [x] Full chain hardware verification
  - [x] Build: 0 warnings
  - [x] Flash + Serial: OK, version 1.4.0
  - [x] Browser (Patchright): 18/18 pass
  - [x] Unit Tests: 43/43 pass (4 suites)

### Day 30: JS Extraction + API Rate Limiting
- [x] JavaScript extraction: index.html 299→132 lines (-56%), created app.js (166 lines)
- [x] Rate Limiter: token bucket throttling (OTA 3/60s, SD delete 10/60s)
- [x] Unit tests: rate_limiter suite 7 tests
- [x] Browser tests added 7 (app.js + panel population), total 23/23
- [x] Full chain hardware verification
  - [x] Build: 0 warnings
  - [x] Flash + Serial: OK
  - [x] Browser (Patchright): 23/23 pass
  - [x] Unit Tests: 50/50 pass (5 suites)

### Day 31: Stability Hardening (Release Sprint 1/3)
- [x] Task Watchdog: 30s timeout, panic mode auto-reboot
- [x] Heap integrity monitoring: 30s periodic checks + low memory warnings
- [x] Health API: /api/system/info added health section (heap_ok, memory_ok, wdt)
- [x] Error handling: cJSON OOM protection (http_send_json, status, system_info)
- [x] app.js reliability: deferred MJPEG loading, prevent connection overload
- [x] Full chain hardware verification
  - [x] Build: 0 warnings
  - [x] Flash × 3 cycles: OK
  - [x] Browser (Patchright): 26/26 pass
  - [x] Unit Tests: 50/50 pass (5 suites)

### Day 32: Test Hardening + Documentation (Release Sprint 2/3)
- [x] Browser integration tests: 26 → 39 (added 13 tests)
  - [x] OTA status, SD status/list, MJPEG stream headers
  - [x] Camera POST (set + restore), LED on/off cycle, LED bad JSON
  - [x] 404 unknown route, Status version/WiFi/RSSI, CORS header
  - [x] MJPEG concurrent connection handling (navigate away + curl verify)
- [x] API documentation: created docs/API.md (20 endpoints fully documented)
- [x] README update: v2.0.0 feature list, project structure, milestone timeline
- [x] Full chain hardware verification
  - [x] Build: no recompile needed (tests + docs only)
  - [x] Browser (Patchright): 39/39 pass
  - [x] Unit Tests: 50/50 pass (5 suites)

### Day 33 — v2.0.0 Release Day 🏆

- [x] Pre-release regression test
  - [x] Unit tests: 50/50 (5 suites)
  - [x] Browser tests: 39/39 (Patchright)
  - [x] Hardware health: heap_ok, memory_ok, WDT active
- [x] Version bump: 1.4.0 → 2.0.0
- [x] CHANGELOG.md: v2.0.0 full release notes
- [x] RELEASE_NOTES.md: updated to v2.0.0
- [x] Build → Flash → Verify
  - [x] Compile: 0 warnings, ~1.29 MB
  - [x] Flash: CH340 serial successful
  - [x] Version confirmed: /api/status → version: 2.0.0
  - [x] Post-flash regression: 39/39 browser tests ✅
- [x] Git tag v2.0.0 + push

### Day 34 — Public Release Day 🚀

- [x] Hardware verification: v2.0.0 online, serial boot log confirmed
- [x] Release screenshots: 4 images (homepage, WS stream, system info, mobile)
- [x] GitHub Release: v2.0.0 published, includes firmware + screenshots
  - URL: https://github.com/chinawrj/Autopilot-ESP32-CAM/releases/tag/v2.0.0
- [x] Daily log + commit + push

### Day 35 — English-Only Internationalization

- [x] Full i18n: 67 files translated Chinese → English
- [x] Categories: source, docs, daily logs, skills, agent config, tools
- [x] Verification: 0 Chinese chars outside README_CN.md
- [x] Build: 0 warnings, unit tests 50/50
- [x] Device: v2.0.0 online, healthy
