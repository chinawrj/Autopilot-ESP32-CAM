#!/usr/bin/env python3
"""Day 19 — Full Quality Audit: test every feature systematically."""

import sys
import json
import time
import urllib.request
import urllib.error

DEVICE = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.171"
BASE = f"http://{DEVICE}"
STREAM = f"http://{DEVICE}:8081"

passed = 0
failed = 0
warnings = 0
results = []

def test(name, ok, detail=""):
    global passed, failed
    if ok:
        passed += 1
        results.append(("PASS", name, detail))
        print(f"  ✅ PASS: {name}" + (f" — {detail}" if detail else ""))
    else:
        failed += 1
        results.append(("FAIL", name, detail))
        print(f"  ❌ FAIL: {name}" + (f" — {detail}" if detail else ""))

def warn(name, detail=""):
    global warnings
    warnings += 1
    results.append(("WARN", name, detail))
    print(f"  ⚠️  WARN: {name}" + (f" — {detail}" if detail else ""))

def http_get(url, timeout=5):
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.status, resp.read(), resp.headers.get("Content-Type", "")

def http_post(url, data, timeout=5):
    body = json.dumps(data).encode() if isinstance(data, dict) else data.encode()
    req = urllib.request.Request(url, data=body, headers={"Content-Type": "application/json"}, method="POST")
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return resp.status, resp.read(), resp.headers.get("Content-Type", "")

print("=" * 50)
print(f" Day 19 — Full Quality Audit")
print(f" Device: {DEVICE}")
print("=" * 50)
print()

# ── 1. Homepage (index.html) ──────────────────────
print("── 1. Homepage (GET /) ──")
try:
    code, body, ct = http_get(f"{BASE}/")
    test("HTTP 200", code == 200, f"code={code}")
    test("Content-Type html", "text/html" in ct, ct)
    test("Page size > 1KB", len(body) > 1000, f"{len(body)} bytes")
    html = body.decode()
    test("Contains AUTOPILOT title", "AUTOPILOT ESP32-CAM" in html)
    test("Contains TCP tab", "TCP Stream" in html)
    test("Contains WS tab", "WS Stream" in html)
    test("Contains LED button", "led-btn" in html)
    test("Contains Snapshot button", "Snapshot" in html)
    test("Contains OTA panel", "OTA Firmware" in html)
    test("Contains System Info", "System Info" in html)
    test("Contains HUD FPS", "hud-fps" in html)
    test("Contains HUD temp", "hud-temp" in html)
    test("MJPEG stream URL points to port 8081", ":8081/stream/tcp" in html)
except Exception as e:
    test("Homepage reachable", False, str(e))
print()

# ── 2. WebSocket page (/stream/ws) ────────────────
print("── 2. WebSocket page (GET /stream/ws) ──")
try:
    code, body, ct = http_get(f"{BASE}/stream/ws")
    test("HTTP 200", code == 200)
    test("Content-Type html", "text/html" in ct)
    ws_html = body.decode()
    test("Contains canvas", "canvas" in ws_html)
    test("Contains WS connect logic", "WebSocket" in ws_html)
    test("Contains quality selector", "set_quality" in ws_html)
    test("Contains resolution selector", "set_resolution" in ws_html)
    test("Contains LED toggle", "toggleLed" in ws_html)
    test("Has back link to /", 'href="/"' in ws_html)
except Exception as e:
    test("WS page reachable", False, str(e))
print()

# ── 3. Status API (/api/status) ───────────────────
print("── 3. Status API (GET /api/status) ──")
try:
    code, body, ct = http_get(f"{BASE}/api/status")
    test("HTTP 200", code == 200)
    test("Content-Type json", "application/json" in ct)
    d = json.loads(body)
    required_fields = ["fps", "temperature", "led_state", "heap_free", "heap_min", "version", "uptime", "rssi", "wifi_connected"]
    for f in required_fields:
        test(f"Field '{f}' present", f in d, f"value={d.get(f)}")
    # Sanity checks
    test("FPS in range [0, 60]", 0 <= d["fps"] <= 60, f"{d['fps']}")
    test("Temperature in range [-10, 60]", -10 <= d["temperature"] <= 60, f"{d['temperature']:.1f}°C")
    test("Heap free > 1MB", d["heap_free"] > 1000000, f"{d['heap_free']/1024:.0f}KB")
    test("Heap min > 1MB", d["heap_min"] > 1000000, f"{d['heap_min']/1024:.0f}KB")
    test("Version is 1.1.0", d["version"] == "1.1.0", d["version"])
    test("Uptime > 0", d["uptime"] > 0, f"{d['uptime']:.0f}s")
    test("RSSI in range [-100, 0]", -100 <= d["rssi"] <= 0, f"{d['rssi']}dBm")
    test("WiFi connected", d["wifi_connected"] == True)
    test("led_state is bool", isinstance(d["led_state"], bool))
except Exception as e:
    test("Status API reachable", False, str(e))
print()

# ── 4. LED API (/api/led) ─────────────────────────
print("── 4. LED Control (POST /api/led) ──")
try:
    # Turn ON
    code, body, ct = http_post(f"{BASE}/api/led", {"state": "on"})
    d = json.loads(body)
    test("LED ON: HTTP 200", code == 200)
    test("LED ON: led_state=true", d["led_state"] == True)

    # Verify via status
    _, sb, _ = http_get(f"{BASE}/api/status")
    sd = json.loads(sb)
    test("LED ON: status confirms", sd["led_state"] == True)

    # Turn OFF
    code, body, ct = http_post(f"{BASE}/api/led", {"state": "off"})
    d = json.loads(body)
    test("LED OFF: led_state=false", d["led_state"] == False)

    # Toggle
    code, body, ct = http_post(f"{BASE}/api/led", {"state": "toggle"})
    d = json.loads(body)
    test("LED TOGGLE: led_state=true", d["led_state"] == True)

    # Toggle back
    code, body, ct = http_post(f"{BASE}/api/led", {"state": "toggle"})
    d = json.loads(body)
    test("LED TOGGLE back: led_state=false", d["led_state"] == False)

    # Invalid state (should not crash)
    code, body, ct = http_post(f"{BASE}/api/led", {"state": "invalid"})
    d = json.loads(body)
    test("Invalid state: no crash", "led_state" in d, f"led_state={d.get('led_state')}")
except Exception as e:
    test("LED API", False, str(e))
print()

# ── 5. LED API error handling ─────────────────────
print("── 5. LED API Error Handling ──")
try:
    # Bad JSON
    try:
        code, body, ct = http_post(f"{BASE}/api/led", "not json")
        test("Bad JSON: returns error", code >= 400, f"code={code}")
    except urllib.error.HTTPError as e:
        test("Bad JSON: HTTP error", e.code == 400, f"code={e.code}")

    # Empty body
    try:
        req = urllib.request.Request(f"{BASE}/api/led", data=b"", headers={"Content-Type": "application/json"}, method="POST")
        with urllib.request.urlopen(req, timeout=5) as resp:
            test("Empty body: returns error", resp.status >= 400)
    except urllib.error.HTTPError as e:
        test("Empty body: HTTP error", e.code == 400, f"code={e.code}")
except Exception as e:
    test("LED error handling", False, str(e))
print()

# ── 6. Snapshot API ──────────────────────────────
print("── 6. Snapshot API (GET /api/snapshot) ──")
try:
    code, body, ct = http_get(f"{BASE}/api/snapshot")
    test("HTTP 200", code == 200)
    test("Content-Type image/jpeg", "image/jpeg" in ct, ct)
    test("Image size > 5KB", len(body) > 5000, f"{len(body)} bytes")
    test("JPEG magic bytes (FFD8)", body[:2] == b'\xff\xd8', f"first 2 bytes: {body[:2].hex()}")
    test("JPEG end marker (FFD9)", body[-2:] == b'\xff\xd9', f"last 2 bytes: {body[-2:].hex()}")

    # Take two snapshots; they should be different (live camera)
    time.sleep(0.5)
    _, body2, _ = http_get(f"{BASE}/api/snapshot")
    test("Two snapshots differ (live)", body != body2, f"snap1={len(body)} snap2={len(body2)}")
except Exception as e:
    test("Snapshot API", False, str(e))
print()

# ── 7. MJPEG Stream (port 8081) ─────────────────
print("── 7. MJPEG Stream (GET :8081/stream/tcp) ──")
try:
    req = urllib.request.Request(f"{STREAM}/stream/tcp")
    with urllib.request.urlopen(req, timeout=5) as resp:
        ct = resp.headers.get("Content-Type", "")
        test("Content-Type multipart", "multipart/x-mixed-replace" in ct, ct)

        # Read a few frames worth of data
        data = resp.read(200000)
        test("Received data > 50KB", len(data) > 50000, f"{len(data)} bytes")

        # Check for JPEG markers in the stream
        jpeg_starts = data.count(b'\xff\xd8')
        test("Contains multiple JPEG frames", jpeg_starts >= 2, f"{jpeg_starts} frames found")

        # Check boundary headers
        test("Contains boundary markers", b"--frame" in data)
except Exception as e:
    test("MJPEG stream", False, str(e))
print()

# ── 8. OTA Status API ───────────────────────────
print("── 8. OTA Status (GET /api/ota/status) ──")
try:
    code, body, ct = http_get(f"{BASE}/api/ota/status")
    test("HTTP 200", code == 200)
    d = json.loads(body)
    test("Field 'in_progress' present", "in_progress" in d, f"{d.get('in_progress')}")
    test("Field 'progress' present", "progress" in d, f"{d.get('progress')}")
    test("Field 'status' present", "status" in d, f"{d.get('status')}")
    test("Field 'version' present", "version" in d, f"{d.get('version')}")
    test("Not in progress", d["in_progress"] == False)
    test("Status is idle", d["status"] == "idle")
except Exception as e:
    test("OTA Status API", False, str(e))
print()

# ── 9. OTA Error Handling ───────────────────────
print("── 9. OTA Error Handling ──")
try:
    # Missing URL
    try:
        code, body, ct = http_post(f"{BASE}/api/ota", {})
        test("Missing URL: returns error", code >= 400)
    except urllib.error.HTTPError as e:
        test("Missing URL: HTTP 400", e.code == 400, f"code={e.code}")

    # Bad JSON
    try:
        code, body, ct = http_post(f"{BASE}/api/ota", "not json")
        test("Bad JSON: returns error", code >= 400)
    except urllib.error.HTTPError as e:
        test("Bad JSON: HTTP 400", e.code == 400, f"code={e.code}")
except Exception as e:
    test("OTA error handling", False, str(e))
print()

# ── 10. 404 handling ────────────────────────────
print("── 10. 404 Handling ──")
try:
    try:
        code, body, ct = http_get(f"{BASE}/nonexistent")
        test("Returns 404", code == 404, f"code={code}")
    except urllib.error.HTTPError as e:
        test("Returns 404", e.code == 404, f"code={e.code}")
except Exception as e:
    test("404 handling", False, str(e))
print()

# ── 11. CORS Headers ────────────────────────────
print("── 11. CORS Headers ──")
try:
    req = urllib.request.Request(f"{BASE}/api/status")
    with urllib.request.urlopen(req, timeout=5) as resp:
        cors = resp.headers.get("Access-Control-Allow-Origin", "")
        test("Status CORS header", cors == "*", f"CORS={cors}")

    req = urllib.request.Request(f"{BASE}/api/snapshot")
    with urllib.request.urlopen(req, timeout=5) as resp:
        cors = resp.headers.get("Access-Control-Allow-Origin", "")
        test("Snapshot CORS header", cors == "*", f"CORS={cors}")
except Exception as e:
    test("CORS headers", False, str(e))
print()

# ── 12. Consistency: LED round-trip ─────────────
print("── 12. LED State Consistency ──")
try:
    # Set to OFF
    http_post(f"{BASE}/api/led", {"state": "off"})
    _, sb, _ = http_get(f"{BASE}/api/status")
    test("status shows OFF after setting OFF", json.loads(sb)["led_state"] == False)

    # Set to ON
    http_post(f"{BASE}/api/led", {"state": "on"})
    _, sb, _ = http_get(f"{BASE}/api/status")
    test("status shows ON after setting ON", json.loads(sb)["led_state"] == True)

    # Reset
    http_post(f"{BASE}/api/led", {"state": "off"})
except Exception as e:
    test("LED consistency", False, str(e))
print()

# ── 13. Temperature sensor behavior ────────────
print("── 13. Temperature Sensor ──")
try:
    temps = []
    for i in range(5):
        _, sb, _ = http_get(f"{BASE}/api/status")
        d = json.loads(sb)
        temps.append(d["temperature"])
        time.sleep(1)
    test("Temperature varies", len(set(f"{t:.2f}" for t in temps)) > 1, f"values={[f'{t:.1f}' for t in temps]}")
    test("All temps in [20, 30]", all(20 <= t <= 30 for t in temps), f"range=[{min(temps):.1f}, {max(temps):.1f}]")
    avg = sum(temps) / len(temps)
    test("Average near 25°C", 22 <= avg <= 28, f"avg={avg:.1f}")
except Exception as e:
    test("Temperature sensor", False, str(e))
print()

# ── 14. FPS updates when streaming ──────────────
print("── 14. FPS Updates During Stream ──")
try:
    # Check FPS before stream
    _, sb, _ = http_get(f"{BASE}/api/status")
    fps_before = json.loads(sb)["fps"]

    # Start a brief MJPEG stream
    req = urllib.request.Request(f"{STREAM}/stream/tcp")
    with urllib.request.urlopen(req, timeout=8) as resp:
        # Read for ~3 seconds
        data = b""
        start = time.time()
        while time.time() - start < 3:
            chunk = resp.read(16384)
            if not chunk:
                break
            data += chunk

    # Check FPS after stream
    time.sleep(1)
    _, sb, _ = http_get(f"{BASE}/api/status")
    fps_after = json.loads(sb)["fps"]
    test("FPS > 0 after streaming", fps_after > 0, f"fps_before={fps_before:.1f}, fps_after={fps_after:.1f}")
except Exception as e:
    test("FPS updates", False, str(e))
print()

# ── 15. Heap stability check ───────────────────
print("── 15. Heap Stability ──")
try:
    heaps = []
    for i in range(3):
        _, sb, _ = http_get(f"{BASE}/api/status")
        d = json.loads(sb)
        heaps.append(d["heap_free"])
        time.sleep(1)
    max_diff = max(heaps) - min(heaps)
    test("Heap stable (drift < 50KB)", max_diff < 50000, f"range={max_diff} bytes, values={[h//1024 for h in heaps]}KB")
    test("Heap > 4MB", heaps[-1] > 4000000, f"{heaps[-1]/1024:.0f}KB")
except Exception as e:
    test("Heap stability", False, str(e))
print()

# ── Summary ─────────────────────────────────────
print()
print("=" * 50)
print(f" QUALITY AUDIT SUMMARY")
print(f" Passed: {passed}")
print(f" Failed: {failed}")
print(f" Warnings: {warnings}")
print(f" Total:  {passed + failed + warnings}")
print("=" * 50)

if failed > 0:
    print("\n FAILED tests:")
    for status, name, detail in results:
        if status == "FAIL":
            print(f"  ❌ {name}: {detail}")

sys.exit(1 if failed > 0 else 0)
