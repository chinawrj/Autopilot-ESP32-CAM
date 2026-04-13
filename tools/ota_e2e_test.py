#!/usr/bin/env python3
"""OTA E2E test — serves firmware and triggers OTA update."""
import http.server
import socketserver
import threading
import time
import json
import urllib.request

LOCAL_IP = "192.168.123.13"
DEVICE_IP = "192.168.1.171"
PORT = 8070
FW_DIR = "/Users/rjwang/fun/Autopilot-ESP32-CAM/build"

import os
os.chdir(FW_DIR)

# Start HTTP server in thread
handler = http.server.SimpleHTTPRequestHandler
httpd = socketserver.TCPServer(("0.0.0.0", PORT), handler)
thread = threading.Thread(target=httpd.serve_forever, daemon=True)
thread.start()
print(f"HTTP server on 0.0.0.0:{PORT}")

# Verify locally
try:
    r = urllib.request.urlopen(f"http://{LOCAL_IP}:{PORT}/autopilot-esp32-cam.bin", timeout=5)
    print(f"Local test: HTTP {r.status}, size={r.headers.get('Content-Length', '?')}B")
    r.close()
except Exception as e:
    print(f"Local test FAILED: {e}")
    httpd.shutdown()
    exit(1)

# Trigger OTA
url = f"http://{LOCAL_IP}:{PORT}/autopilot-esp32-cam.bin"
print(f"\nTriggering OTA: {url}")
req = urllib.request.Request(
    f"http://{DEVICE_IP}/api/ota",
    data=json.dumps({"url": url}).encode(),
    headers={"Content-Type": "application/json"},
    method="POST"
)
try:
    resp = urllib.request.urlopen(req, timeout=5)
    print(f"OTA trigger: {resp.read().decode()}")
except Exception as e:
    print(f"OTA trigger failed: {e}")
    httpd.shutdown()
    exit(1)

# Monitor progress
print("\nMonitoring OTA progress...")
passed = False
for i in range(1, 60):
    time.sleep(5)
    try:
        r = urllib.request.urlopen(f"http://{DEVICE_IP}/api/ota/status", timeout=3)
        data = json.loads(r.read().decode())
        r.close()
        st = data.get("status", "?")
        pct = data.get("progress", 0)
        print(f"  [{i:2d}] status={st} progress={pct}%")
        
        if st == "idle" and i > 5:
            print("\n=== OTA COMPLETE — device rebooted! ===")
            time.sleep(2)
            r2 = urllib.request.urlopen(f"http://{DEVICE_IP}/api/status", timeout=5)
            health = json.loads(r2.read().decode())
            r2.close()
            print(f"  Version: {health.get('version')}")
            print(f"  Uptime:  {health.get('uptime')}s")
            print(f"  Heap:    {health.get('heap_free', 0) // 1024}KB")
            print(f"  FPS:     {health.get('fps', 0):.1f}")
            print(f"  WiFi:    {'connected' if health.get('wifi_connected') else 'disconnected'}")
            passed = True
            break
        
        if "error" in st:
            print(f"\n=== OTA FAILED: {st} ===")
            break
    except Exception:
        print(f"  [{i:2d}] unreachable (rebooting?)")

httpd.shutdown()
print(f"\nOTA E2E TEST: {'PASSED' if passed else 'FAILED'}")
