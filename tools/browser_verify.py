#!/usr/bin/env python3
"""Browser verification for ESP32-CAM unified dashboard (Day 17)."""
import os, signal, time
from patchright.sync_api import sync_playwright

DEVICE_IP = "192.168.1.171"
USER_DATA_DIR = os.path.expanduser("~/.patchright-userdata")

pw = sync_playwright().start()
context = pw.chromium.launch_persistent_context(
    user_data_dir=USER_DATA_DIR,
    channel="chrome",
    headless=False,
    no_viewport=True,
    ignore_default_args=["--no-sandbox"],
)
page = context.pages[0] if context.pages else context.new_page()

print("=== Day 17 Browser Verification ===")

# 1. Load unified dashboard
print("\n[1] Loading unified dashboard...")
page.goto(f"http://{DEVICE_IP}/", wait_until="domcontentloaded", timeout=15000)
time.sleep(6)

tabs = page.query_selector_all(".tab")
print(f"  Tabs: {[t.inner_text() for t in tabs]}")

# Check elements exist
for sel, name in [("#tcp-view", "TCP img"), ("#ws-canvas", "WS canvas"),
                  ("#led-btn", "LED btn"), ("button.snap", "Snapshot btn"),
                  ("#si-uptime", "Uptime"), ("#si-heap", "Heap"),
                  ("#si-rssi", "RSSI"), ("#fw-ver", "FW ver")]:
    el = page.query_selector(sel)
    txt = el.inner_text() if el else "MISSING"
    print(f"  {name}: {txt[:40]}")

page.screenshot(path="/tmp/day17_tcp.png")
print("  Screenshot: /tmp/day17_tcp.png")

# 2. Switch to WS mode
print("\n[2] WebSocket mode...")
if len(tabs) > 1:
    tabs[1].click()
    time.sleep(5)
    ws_stat = page.inner_text("#hud-ws")
    print(f"  WS: {ws_stat}")
    page.screenshot(path="/tmp/day17_ws.png")
    print("  Screenshot: /tmp/day17_ws.png")

# 3. Switch back to TCP
print("\n[3] Back to TCP, checking snapshot + LED...")
tabs[0].click()
time.sleep(2)
snap = page.query_selector("button.snap")
led = page.query_selector("#led-btn")
print(f"  Snapshot: {snap.inner_text() if snap else 'MISSING'}")
print(f"  LED: {led.inner_text() if led else 'MISSING'}")

print("\n=== Done — browser stays open ===")
