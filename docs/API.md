# API Reference

Autopilot ESP32-CAM exposes a REST API on **port 80** and an MJPEG stream on **port 8081**.

All JSON endpoints return `Content-Type: application/json` with security headers
(`X-Content-Type-Options: nosniff`, `X-Frame-Options: DENY`, `Cache-Control: no-store`).

---

## Pages

### `GET /`

Serve the main dashboard (embedded gzip HTML).

**Response:** `text/html`

### `GET /app.js`

Serve the frontend JavaScript (embedded gzip). Cached for 1 hour.

**Response:** `application/javascript`, `Cache-Control: public, max-age=3600`

### `GET /stream/ws`

Serve the WebSocket stream viewer page (embedded gzip HTML).

**Response:** `text/html`

---

## Status & System

### `GET /api/status`

Quick system status for dashboard polling.

**Response:**
```json
{
  "fps": 25.0,
  "temperature": 53.3,
  "led_state": false,
  "heap_free": 4168000,
  "heap_min": 4100000,
  "version": "1.4.0",
  "uptime": 12345,
  "rssi": -45,
  "wifi_connected": true
}
```

### `GET /api/system/info`

Comprehensive system diagnostics including hardware info and health status.

**Response:**
```json
{
  "chip": {
    "model": "ESP32",
    "cores": 2,
    "revision": 301,
    "wifi": true,
    "bt": true
  },
  "idf_version": "v5.5.1",
  "memory": {
    "heap_free": 4168000,
    "heap_min": 4100000,
    "psram_free": 4038656,
    "psram_total": 4194304
  },
  "uptime_s": 12345,
  "uptime": "0d 03:25:45",
  "task_count": 15,
  "rssi": -45,
  "wifi_connected": true,
  "ws_fps": 0.0,
  "mjpeg_fps": 25.0,
  "health": {
    "heap_ok": true,
    "wdt_timeout_s": 30,
    "internal_free": 137000,
    "memory_ok": true
  }
}
```

---

## Camera

### `GET /api/camera`

Get current camera settings.

**Response:**
```json
{
  "brightness": 0,
  "contrast": 0,
  "saturation": 0,
  "sharpness": 0,
  "hmirror": 0,
  "vflip": 0,
  "quality": 12,
  "framesize": 8
}
```

### `POST /api/camera`

Update camera settings. All fields are optional.

**Request:**
```json
{
  "brightness": 0,
  "contrast": 0,
  "saturation": 0,
  "sharpness": 0,
  "hmirror": 0,
  "vflip": 0,
  "quality": 12
}
```

**Response:** Updated settings (same format as GET).

| Status | Meaning |
|--------|---------|
| 200 | Settings applied |
| 400 | Invalid JSON |
| 500 | Camera sensor error |

### `GET /api/snapshot`

Capture a single JPEG frame from the camera.

**Response:** `image/jpeg` binary data with `Access-Control-Allow-Origin: *`.

| Status | Meaning |
|--------|---------|
| 200 | JPEG image |
| 500 | Camera capture failed |

---

## LED Control

### `POST /api/led`

Control the onboard LED (GPIO 33).

**Request:**
```json
{ "state": "on" }
```

Valid values: `"on"`, `"off"`, `"toggle"`.

**Response:**
```json
{ "led_state": true }
```

| Status | Meaning |
|--------|---------|
| 200 | LED state changed |
| 400 | Invalid JSON or missing `state` field |

---

## Video Streaming

### `GET /ws/stream` (WebSocket)

WebSocket endpoint for live camera stream. Connect with a WebSocket client;
the server pushes binary JPEG frames continuously.

### `GET /stream/tcp` (Port 8081)

HTTP MJPEG stream. Returns `multipart/x-mixed-replace;boundary=frame`.
Each part is a JPEG frame. The connection stays open until the client disconnects.

**Response headers:**
```
Content-Type: multipart/x-mixed-replace;boundary=frame
Access-Control-Allow-Origin: *
X-Framerate: 25
```

---

## SD Card

### `GET /api/sd/status`

Check if the SD card is mounted and its capacity.

**Response (mounted):**
```json
{
  "mounted": true,
  "name": "SD",
  "total_bytes": 15931539456,
  "free_bytes": 15905742848
}
```

**Response (not mounted):**
```json
{ "mounted": false }
```

### `GET /api/sd/list`

List files in a directory on the SD card.

**Query parameters:** `path` (optional, default `/`).

**Response:**
```json
{
  "mounted": true,
  "files": [
    { "name": "photo_001.jpg", "size": 45678, "dir": false },
    { "name": "logs", "size": 0, "dir": true }
  ]
}
```

### `POST /api/sd/capture`

Capture a photo and save it to the SD card.

**Response:**
```json
{
  "ok": true,
  "filename": "/sdcard/photo_001.jpg",
  "size": 45678
}
```

| Status | Meaning |
|--------|---------|
| 200 | Photo saved |
| 500 | Camera or SD write error |

### `POST /api/sd/delete`

Delete a file from the SD card. **Rate limited:** 10 requests per 60 seconds.

**Request:**
```json
{ "filename": "photo_001.jpg" }
```

**Response:**
```json
{ "ok": true }
```

| Status | Meaning |
|--------|---------|
| 200 | File deleted |
| 400 | Invalid JSON or missing filename |
| 429 | Rate limit exceeded |

### `GET /api/sd/file/*`

Download a file from the SD card. The path after `/api/sd/file/` specifies the file.
Path traversal attempts (e.g., `../`) are blocked.

**Example:** `GET /api/sd/file/photo_001.jpg`

**Response:** `image/jpeg` binary data (chunked transfer).

| Status | Meaning |
|--------|---------|
| 200 | File content |
| 404 | File not found or SD not mounted |

---

## OTA Update

### `POST /api/ota`

Start an over-the-air firmware update. **Rate limited:** 3 requests per 60 seconds.

**Request:**
```json
{ "url": "https://example.com/firmware.bin" }
```

**Response:**
```json
{ "status": "started" }
```

| Status | Meaning |
|--------|---------|
| 200 | OTA started |
| 400 | Invalid JSON or missing URL |
| 429 | Rate limit exceeded |

### `GET /api/ota/status`

Check OTA update progress.

**Response:**
```json
{
  "in_progress": false,
  "progress": 0,
  "status": "idle",
  "version": "1.4.0"
}
```

---

## Debug

### `POST /api/debug/wifi-disconnect`

Force disconnect from WiFi (for testing reconnection logic).

**Response:**
```json
{ "status": "disconnected" }
```
