#!/usr/bin/env python3
"""Test WiFi auto-reconnect by forcing disconnect via debug endpoint."""
import sys, time, urllib.request, json

IP = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.171"
BASE = f"http://{IP}"

def api_get(path, timeout=3):
    try:
        with urllib.request.urlopen(f"{BASE}{path}", timeout=timeout) as r:
            return json.loads(r.read())
    except Exception:
        return None

def api_post(path, timeout=3):
    try:
        req = urllib.request.Request(f"{BASE}{path}", method="POST", data=b"")
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return json.loads(r.read())
    except Exception:
        return None

print("=== WiFi Disconnect & Reconnect Test ===")
print()

# Step 1: verify online
print("Step 1: Verify device is online")
status = api_get("/api/status")
if not status:
    print("  Device NOT reachable - abort")
    sys.exit(1)
print(f"  Online: heap={status['heap_free']}, temp={status['temperature']:.1f}")
print()

# Step 2: force disconnect
print("Step 2: Force WiFi disconnect")
resp = api_post("/api/debug/wifi-disconnect")
print(f"  Response: {resp}")
print()

# Step 3: verify unreachable
print("Step 3: Wait for device to be unreachable...")
time.sleep(2)
status = api_get("/api/status", timeout=2)
if status:
    print("  Still reachable (may not have disconnected yet)")
else:
    print("  Device unreachable (WiFi down) OK")
print()

# Step 4: wait for reconnect
print("Step 4: Wait for auto-reconnect (up to 20s)...")
for i in range(1, 21):
    time.sleep(1)
    status = api_get("/api/status", timeout=1)
    if status:
        print(f"  Reconnected after {i}s!")
        print(f"  heap={status['heap_free']}, temp={status['temperature']:.1f}")
        print()
        print("PASS: WiFi auto-reconnect works")
        sys.exit(0)

print("  NOT reconnected after 20s")
print("FAIL: WiFi auto-reconnect did not work")
sys.exit(1)
