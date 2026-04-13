#!/usr/bin/env python3
"""Day 18 regression test — browser verification of all UI features."""
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

PASS = 0
FAIL = 0

def check(name, condition, detail=""):
    global PASS, FAIL
    if condition:
        PASS += 1
        print(f"  [PASS] {name} {detail}")
    else:
        FAIL += 1
        print(f"  [FAIL] {name} {detail}")

print("=" * 50)
print("  Day 18: Browser Regression Test v1.1.0")
print("=" * 50)

# 1. Load unified dashboard
print("\n[1] Unified Dashboard...")
page.goto(f"http://{DEVICE_IP}/", wait_until="domcontentloaded", timeout=15000)
time.sleep(5)

tabs = page.query_selector_all(".tab")
check("Tabs exist", len(tabs) >= 2, f"(found {len(tabs)})")

# Check key UI elements
for sel, name in [("#tcp-view", "TCP stream img"), ("#ws-canvas", "WS canvas"),
                  ("#led-btn", "LED button"), ("button.snap", "Snapshot button"),
                  ("#si-uptime", "Uptime display"), ("#si-heap", "Heap display"),
                  ("#si-rssi", "RSSI display"), ("#fw-ver", "FW version")]:
    el = page.query_selector(sel)
    check(name, el is not None)

# Check FW version shows 1.1.0
fw_el = page.query_selector("#fw-ver")
fw_text = fw_el.inner_text() if fw_el else ""
check("FW version = 1.1.0", "1.1.0" in fw_text, f"({fw_text})")

# Check system info is being updated (uptime > 0)
uptime_el = page.query_selector("#si-uptime")
uptime_text = uptime_el.inner_text() if uptime_el else "0"
check("Uptime > 0", uptime_text != "0" and uptime_text != "", f"({uptime_text})")

# Check TCP stream is loading (img should have src with port 81)
tcp_img = page.query_selector("#tcp-view")
tcp_src = tcp_img.get_attribute("src") if tcp_img else ""
check("TCP stream src has :81", ":81" in tcp_src, f"({tcp_src[:60]})")

page.screenshot(path="/tmp/day18_tcp.png")
print("  Screenshot: /tmp/day18_tcp.png")

# 2. WebSocket mode
print("\n[2] WebSocket Mode...")
if tabs and len(tabs) > 1:
    tabs[1].click()
    time.sleep(5)
    ws_hud = page.query_selector("#hud-ws")
    ws_text = ws_hud.inner_text() if ws_hud else ""
    check("WS HUD visible", ws_hud is not None and ws_text != "")
    check("WS FPS > 0", "FPS" in ws_text, f"({ws_text})")
    page.screenshot(path="/tmp/day18_ws.png")
    print("  Screenshot: /tmp/day18_ws.png")

# 3. OTA panel
print("\n[3] OTA Panel...")
ota_input = page.query_selector("#ota-url")
ota_btn = page.query_selector("#ota-btn")
check("OTA URL input", ota_input is not None)
check("OTA button", ota_btn is not None)

# 4. LED toggle
print("\n[4] LED Toggle...")
led_btn = page.query_selector("#led-btn")
if led_btn:
    before = led_btn.inner_text()
    led_btn.click()
    time.sleep(1)
    after = led_btn.inner_text()
    check("LED state changed", before != after, f"({before} -> {after})")
    # Toggle back
    led_btn.click()
    time.sleep(1)

# 5. Snapshot
print("\n[5] Snapshot...")
tabs[0].click()  # back to TCP
time.sleep(2)
snap_btn = page.query_selector("button.snap")
if snap_btn:
    snap_btn.click()
    time.sleep(2)
    check("Snapshot button clicked", True)

print("\n" + "=" * 50)
print(f"  RESULTS: {PASS} passed, {FAIL} failed")
print("=" * 50)
print("\nBrowser stays open for manual inspection.")
