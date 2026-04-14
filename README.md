# Autopilot ESP32-CAM

[![CI](https://github.com/chinawrj/Autopilot-ESP32-CAM/actions/workflows/ci.yml/badge.svg)](https://github.com/chinawrj/Autopilot-ESP32-CAM/actions/workflows/ci.yml)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-blue?logo=espressif)](https://docs.espressif.com/projects/esp-idf/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32--WROVER--E-orange?logo=espressif)](https://www.espressif.com/en/products/socs/esp32)
[![Built by](https://img.shields.io/badge/Built%20by-Claude%20Opus%204.6-blueviolet?logo=anthropic)](https://www.anthropic.com/claude)

**[中文文档 →](README_CN.md)**

A real-time camera web server built on the **YD-ESP32-CAM** (ESP32-WROVER-E-N8R8) development board. Supports dual-path video streaming (TCP MJPEG + WebSocket), real-time HUD overlay, and remote LED control. Developed entirely by an AI Agent through daily iterations — from zero to delivery.

<p align="center">
  <img src="docs/images/mjpeg-stream.png" width="48%" alt="MJPEG Stream with HUD">
  <img src="docs/images/ws-stream.png" width="48%" alt="WebSocket Stream with Controls">
</p>

---

## Features

| Feature | Description |
|---------|-------------|
| **TCP Video Stream** | MJPEG over HTTP at `/stream/tcp` — works with any `<img>` tag |
| **WebSocket Video Stream** | Binary JPEG push at `/ws/stream` with Canvas rendering, up to 4 concurrent clients |
| **Real-time HUD** | FPS counter + virtual temperature sensor (25°C ±3°C) overlaid on video |
| **WebSocket Control** | Dynamic quality (Q10-Q50), resolution (QVGA/VGA/SVGA/XGA) adjustment |
| **LED Control** | Web button to toggle onboard LED (GPIO33) on/off |
| **OTA Firmware Update** | Over-the-air upgrade via `POST /api/ota` with progress tracking and rollback protection |
| **Snapshot API** | `GET /api/snapshot` returns a single JPEG frame for capture/download |
| **Unified Dashboard** | Single-page UI with tab switching (TCP / WebSocket), all controls on one page |
| **Heartbeat** | 5-second periodic heartbeat with FPS, client count, and heap memory stats |
| **WiFi Auto-Reconnect** | Exponential backoff reconnection (1s → 10s), infinite retry after initial connect |
| **mDNS Discovery** | Access via `http://espcam.local/` — no need to remember IP addresses |
| **Camera Settings** | Real-time camera parameter adjustment (brightness, contrast, saturation, mirror, flip) via `/api/camera` |
| **SD Card Storage** | Micro SD card support: capture JPEG to SD, browse/download/delete files via `/api/sd/*` |
| **Unit Tests** | 20 host-based unit tests (fps_counter, virtual_sensor, led_controller) via Unity framework |
| **Heap Monitoring** | `/api/status` returns real-time heap info with RSSI, uptime; 30s serial logging |

## Hardware

| Parameter | Value |
|-----------|-------|
| **Board** | YD-ESP32-CAM (VCC-GND Studio) |
| **Module** | ESP32-WROVER-E-N8R8 (8MB Flash + 8MB PSRAM) |
| **SoC** | ESP32-D0WD-V3 (Dual-core Xtensa LX6, 240MHz) |
| **Camera** | OV2640 (VGA 640×480, JPEG q=12) |
| **Onboard LED** | GPIO33 |
| **Serial Chip** | CH340 |

### GPIO Pinout

#### Camera (OV2640)

| Signal | GPIO | Signal | GPIO |
|--------|------|--------|------|
| D0 | 5 | D4 | 36 (input-only) |
| D1 | 18 | D5 | 39 (input-only) |
| D2 | 19 | D6 | 34 (input-only) |
| D3 | 21 | D7 | 35 (input-only) |
| XCLK | 0 | PCLK | 22 |
| VSYNC | 25 | HREF | 23 |
| SDA (SIOD) | 26 | SCL (SIOC) | 27 |
| PWDN | 32 | RESET | -1 (N/A) |

#### SD Card

| Signal | GPIO | Signal | GPIO |
|--------|------|--------|------|
| CLK | 14 | DATA0 | 2 |
| CMD | 15 | DATA1 | 4 |
| DATA2 | 12 ⚠️ | DATA3 | 13 |

#### Other

| Function | GPIO | Note |
|----------|------|------|
| Onboard LED | 33 | Active LOW |
| BOOT Button | 0 | Shared with XCLK |
| U0TXD | 1 | Serial TX |
| U0RXD | 3 | Serial RX |

> **Key Conflicts:** GPIO0 = XCLK + BOOT (disconnect camera for flashing) · GPIO4 = Flash LED + SD DAT1 · GPIO12 = SD DAT2 + MTDI (run `espefuse.py set_flash_voltage 3.3V`) · GPIO34-39 are input-only.

## Architecture

```mermaid
graph TB
    subgraph Browser["🌐 Browser Client"]
        A1["📺 Unified Dashboard<br/>index.html (TCP / WS tabs)"]
    end

    subgraph ESP["⚡ ESP32-CAM"]
        subgraph HTTP80["HTTP Server :80 (API + Pages + WS)"]
            B1["GET /"]
            B4["GET /api/status"]
            B5["POST /api/led"]
            B6["WS /ws/stream"]
            B7["GET /api/snapshot"]
            B8["POST /api/ota"]
            B9["SD /api/sd/*"]
        end

        subgraph HTTP8081["Stream Server :8081 (MJPEG)"]
            B2["GET /stream/tcp"]
        end

        subgraph Modules["Core Modules"]
            C1["wifi_manager"]
            C2["camera_init"]
            C3["ws_stream"]
            C4["led_controller"]
            C5["virtual_sensor"]
            C6["ota_update"]
            C7["sd_card"]
        end

        subgraph HW["Hardware"]
            D2["OV2640"]
            D3["8MB PSRAM"]
            D4["GPIO33 LED"]
            D5["WiFi Radio"]
            D6["Dual OTA Flash"]
            D7["Micro SD Card"]
        end
    end

    A1 -- "HTTP :8081" --> B2
    A1 -- "WebSocket" --> B6
    A1 -- "fetch" --> B4 & B7 & B9
    A1 -- "POST" --> B5 & B8

    B2 --> C2
    B6 --> C3 --> C2
    B7 --> C2
    B9 --> C7 --> D7
    B4 --> C5
    B5 --> C4
    B8 --> C6
    C1 --> D5
    C2 --> D2 & D3
    C4 --> D4
    C6 --> D6
```

## Quick Start

### 1. Prerequisites

- [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- USB-TTL adapter (CH340/CP2102/FTDI) connected to GPIO1 (TX) / GPIO3 (RX)

```bash
. $HOME/esp/esp-idf/export.sh
```

### 2. Configure WiFi Credentials

> ⚠️ WiFi credentials are **never** stored in the repository.

```bash
# Option A: Environment variables
export ESP_WIFI_SSID="YourSSID"
export ESP_WIFI_PASSWORD="YourPassword"

# Option B: Secure config file (recommended)
cat > ~/.esp-wifi-credentials << 'EOF'
[wifi]
ssid = YourSSID
password = YourPassword
EOF
chmod 600 ~/.esp-wifi-credentials

# Inject credentials into build config
bash tools/provision-wifi.sh
```

### 3. Build & Flash

```bash
idf.py build
idf.py -p /dev/cu.wchusbserial110 flash monitor
```

### 4. Open Web Interface

After WiFi connection, the serial output shows the device IP:

```
I (2380) wifi_mgr: WiFi connected, IP: 192.168.1.171
I (2630) main: System ready — http://192.168.1.171/
```

| Page | URL | Description |
|------|-----|-------------|
| Dashboard | `http://<IP>/` | Unified UI with TCP/WS tab switching |
| MJPEG Stream | `http://<IP>:8081/stream/tcp` | Direct MJPEG video (also embedded in dashboard) |
| Status API | `http://<IP>/api/status` | JSON: fps, temperature, heap, rssi, uptime |
| LED Control | `POST http://<IP>/api/led` | Body: `{"state":"on/off/toggle"}` |
| Snapshot | `http://<IP>/api/snapshot` | Single JPEG frame capture |
| OTA Update | `POST http://<IP>/api/ota` | Body: `{"url":"http://..."}` |
| OTA Status | `http://<IP>/api/ota/status` | OTA progress and state |
| SD Status | `http://<IP>/api/sd/status` | SD card mount state + capacity |
| SD File List | `http://<IP>/api/sd/list` | List files on SD card |
| SD Capture | `POST http://<IP>/api/sd/capture` | Capture JPEG to SD card |

## Web Interface

### Unified Dashboard

Single-page application with tab switching between TCP MJPEG and WebSocket streams. Includes HUD overlay (FPS + temperature), LED control, snapshot download, OTA firmware update, and system info panel.

<p align="center">
  <img src="docs/images/mjpeg-stream.png" width="48%" alt="MJPEG Stream with HUD">
  <img src="docs/images/ws-stream.png" width="48%" alt="WebSocket Stream with Controls">
</p>

## API Reference

### GET `/api/status`

```json
{
  "fps": 10.5,
  "temperature": 25.3,
  "led_state": false,
  "heap_free": 4224764,
  "heap_min": 4161592,
  "rssi": -45,
  "uptime": 3600,
  "wifi_connected": true,
  "version": "1.3.0"
}
```

### POST `/api/led`

```bash
# Toggle LED
curl -X POST http://192.168.1.171/api/led -d '{"state":"toggle"}'

# Turn ON / OFF
curl -X POST http://192.168.1.171/api/led -d '{"state":"on"}'
curl -X POST http://192.168.1.171/api/led -d '{"state":"off"}'
```

### WebSocket `/ws/stream`

**Binary frames**: JPEG image data
**Text frames** (heartbeat, every 5s):
```json
{
  "type": "heartbeat",
  "fps": 10.5,
  "clients": 2,
  "heap_free": 4224764,
  "heap_min": 4161592
}
```

**Control messages** (client → server):
```json
{"action": "set_quality", "value": 20}
{"action": "set_resolution", "value": "SVGA"}
{"action": "get_status"}
```

### GET `/api/snapshot`

Returns a single JPEG frame (`Content-Type: image/jpeg`).

```bash
curl -o snapshot.jpg http://192.168.1.171/api/snapshot
```

### POST `/api/ota`

```bash
curl -X POST http://192.168.1.171/api/ota -d '{"url":"http://192.168.1.100:8070/firmware.bin"}'
```

### GET `/api/camera`

Returns current camera sensor settings.

```json
{"brightness": 0, "contrast": 0, "saturation": 0, "sharpness": 0,
 "hmirror": false, "vflip": false, "quality": 12, "framesize": 10}
```

### POST `/api/camera`

Update camera settings (partial JSON accepted — send only changed fields).

```bash
curl -X POST http://192.168.1.171/api/camera -d '{"brightness":1,"vflip":true}'
```

### GET `/api/ota/status`

```json
{"status": "idle", "progress": 0, "message": ""}
```

### GET `/api/sd/status`

```json
{"mounted": true, "name": "SD32G", "total_bytes": 31914983424, "free_bytes": 31903055872}
```

### GET `/api/sd/list`

```json
{"files": [{"name": "img_001.jpg", "size": 12345}]}
```

### POST `/api/sd/capture`

Captures a JPEG snapshot and saves it to the SD card.

```bash
curl -X POST http://192.168.1.171/api/sd/capture
# Response: {"filename": "img_001.jpg", "size": 12345}
```

### GET `/api/sd/file/{filename}`

Downloads a file from the SD card (`Content-Type: image/jpeg` for `.jpg`).

```bash
curl -o photo.jpg http://192.168.1.171/api/sd/file/img_001.jpg
```

### POST `/api/sd/delete`

```bash
curl -X POST http://192.168.1.171/api/sd/delete -d '{"filename":"img_001.jpg"}'
```

## Performance

| Metric | Value |
|--------|-------|
| MJPEG FPS | ~10 fps (VGA, 1 client) |
| WebSocket FPS | ~10 fps (VGA, 1 client) |
| Multi-client | 3 WS + 1 MJPEG simultaneous, 0 errors |
| JPEG Frame Size | ~10-15 KB (VGA, q=12) |
| Free Heap | ~4.2 MB (with PSRAM) |
| Firmware Size | ~1.2 MB (dual OTA partition) |
| Partition Layout | Dual OTA (3MB × 2) + 1.97MB SPIFFS |
| WiFi Reconnect | Auto, 1-10s exponential backoff |
| Total C Code | ~1600 lines across 13 source files |
| Unit Tests | 20/20 (3 test suites, host-based) |

## Project Structure

```
├── main/
│   ├── main.c              # Entry point, init chain + heap logging
│   ├── wifi_manager.c/h    # WiFi STA management, auto-reconnect
│   ├── camera_init.c/h     # OV2640 camera initialization + I2C recovery
│   ├── http_server.c/h     # HTTP server :80, API routes, WebSocket upgrade
│   ├── http_helpers.c/h    # JSON response helper (http_send_json)
│   ├── stream_server.c/h   # Independent MJPEG stream server :8081
│   ├── ws_stream.c/h       # WebSocket video stream + control messages
│   ├── ota_update.c/h      # OTA firmware update with rollback protection
│   ├── led_controller.c/h  # GPIO33 LED driver
│   ├── sd_handlers.c/h     # SD card REST API handlers (5 endpoints)
│   ├── index.html          # Unified dashboard (TCP/WS tabs + all controls)
│   └── stream_ws.html      # WebSocket stream standalone page
├── components/
│   ├── virtual_sensor/     # Virtual temperature sensor component
│   ├── fps_counter/        # Reusable FPS calculation component
│   └── sd_card/            # SD card driver (1-bit SDMMC + VFS FAT)
├── test/
│   ├── unit/               # Unit tests (fps_counter, virtual_sensor, led_controller)
│   ├── mocks/              # ESP-IDF mock layer for host-based testing
│   └── CMakeLists.txt      # Host build: cmake && make && ctest
├── tools/
│   ├── provision-wifi.sh       # WiFi credential injection
│   ├── quality_audit.py        # API test suite (80 tests)
│   ├── browser_qa.py           # Browser UI test suite (50 tests)
│   ├── regression_test.py      # Browser regression test
│   ├── ota_e2e_test.py         # OTA end-to-end test
│   ├── heap_monitor.py         # Heap memory trend monitor
│   ├── multi_client_test.py    # Multi-client stress test
│   ├── wifi_reconnect_test.py  # WiFi reconnect test
│   ├── browser_verify.py       # Browser automation verification
│   └── take_screenshots.py     # Release screenshot tool
├── docs/
│   ├── TARGET.md           # Milestone tracking
│   ├── images/             # Screenshots for documentation
│   └── daily-logs/         # Daily development logs (Day 001–024)
├── CHANGELOG.md            # Version history
├── sdkconfig.defaults      # ESP-IDF default configuration
├── partitions.csv          # Partition table (dual OTA 3MB×2 + 1.97MB SPIFFS)
└── CMakeLists.txt
```

## Development Story

### About the Developer

This project was **autonomously developed by an AI Agent** — specifically **Claude Opus 4.6** (Anthropic), operating as a senior embedded engineer inside VS Code with GitHub Copilot. No human wrote any firmware code; the AI Agent completed the entire development lifecycle independently:

- **Planning**: Read hardware datasheets, defined milestones, and created daily task lists
- **Coding**: Wrote all C firmware (ESP-IDF), HTML/JS frontends, and Python test tools
- **Testing**: Compiled, flashed to real hardware, read serial logs, and verified via browser — every single change
- **Debugging**: Diagnosed crash backtraces, fixed memory issues, resolved WiFi reconnection edge cases
- **Documentation**: Wrote bilingual README, CHANGELOG, architecture diagrams, and daily logs
- **Release**: Created GitHub Release, took screenshots via browser automation (Patchright)

The human's role was limited to: connecting the hardware, providing WiFi credentials, and relaying customer feedback.

### Milestone Timeline

| Milestone | Day | Deliverable |
|-----------|-----|-------------|
| M0: Scaffold | Day 1 | ESP-IDF project + WiFi management |
| M1: TCP Stream | Day 3 | MJPEG over HTTP |
| M2: HUD Overlay | Day 5 | FPS + temperature overlay |
| M3: LED Control | Day 4 | GPIO33 web control |
| M4: WebSocket Stream | Day 8 | WS video + control messages + heartbeat |
| M5: Stability | Day 11 | Memory leak tests + stress tests + WiFi reconnect |
| Release v1.0.0 | Day 13 | Bilingual docs + screenshots + GitHub Release |
| M6: OTA + Dashboard | Day 18 | OTA update + snapshot + unified dashboard |
| Release v1.1.0 | Day 18 | Dual server architecture + OTA + unified UI |
| Quality Audit | Day 19 | 130/130 tests passed, port 81→8081 fix |
| Release v1.1.1 | Day 20 | Documentation refresh + patch release |
| Release v1.2.0 | Day 21 | mDNS discovery + Camera Settings API |
| Refactoring | Day 22 | FPS component + JSON helper + 20 unit tests |
| Release v1.3.0 | Day 23 | SD Card Storage + Unit Test framework |

Every code change was build → flash → serial verify → browser verify on real hardware. See [docs/daily-logs/](docs/daily-logs/) for detailed development logs.

## License

MIT
