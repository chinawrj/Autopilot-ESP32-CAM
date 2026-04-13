#!/usr/bin/env python3
"""Day 19 — Browser Visual QA: verify every UI feature in Chrome via Patchright."""

import os
import sys
import time
import json

sys.path.insert(0, os.path.expanduser("~/patchright-env/lib/python3.13/site-packages"))
from patchright.sync_api import sync_playwright

DEVICE = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.171"
BASE = f"http://{DEVICE}"
USER_DATA_DIR = os.path.expanduser("~/.patchright-userdata")
SCREENSHOT_DIR = "/tmp/day19"
os.makedirs(SCREENSHOT_DIR, exist_ok=True)

passed = 0
failed = 0
results = []

def test(name, ok, detail=""):
    global passed, failed
    if ok:
        passed += 1
        results.append(("PASS", name, detail))
        print(f"  ✅ {name}" + (f" — {detail}" if detail else ""))
    else:
        failed += 1
        results.append(("FAIL", name, detail))
        print(f"  ❌ {name}" + (f" — {detail}" if detail else ""))

pw = sync_playwright().start()
context = pw.chromium.launch_persistent_context(
    user_data_dir=USER_DATA_DIR,
    channel="chrome",
    headless=False,
    no_viewport=True,
    ignore_default_args=["--no-sandbox"],
)
page = context.pages[0] if context.pages else context.new_page()

print("=" * 55)
print(f" Day 19 — Browser Visual QA")
print(f" Device: {DEVICE}")
print("=" * 55)
print()

# ── 1. Dashboard loads correctly ──
print("── 1. Dashboard Page ──")
page.goto(f"{BASE}/", wait_until="networkidle", timeout=15000)
time.sleep(2)

test("Title contains ESP32-CAM", "ESP32-CAM" in page.title())
test("AUTOPILOT heading visible", page.locator("h1").is_visible())
heading_text = page.locator("h1").inner_text()
test("Heading says AUTOPILOT", "AUTOPILOT" in heading_text, heading_text)

# Tabs
tcp_tab = page.locator(".tab").first
ws_tab = page.locator(".tab").nth(1)
test("TCP tab visible", tcp_tab.is_visible())
test("WS tab visible", ws_tab.is_visible())
test("TCP tab is active by default", "active" in tcp_tab.get_attribute("class"))

page.screenshot(path=f"{SCREENSHOT_DIR}/01_dashboard_load.png")
print()

# ── 2. TCP MJPEG Stream ──
print("── 2. TCP MJPEG Stream ──")
tcp_img = page.locator("#tcp-view")
test("TCP img element present", tcp_img.is_visible())
src = tcp_img.get_attribute("src")
test("Img src points to :8081/stream/tcp", ":8081/stream/tcp" in src, src)

# Wait for stream to render
time.sleep(3)
# Check image loaded (naturalWidth > 0)
nat_w = page.evaluate("document.getElementById('tcp-view').naturalWidth")
nat_h = page.evaluate("document.getElementById('tcp-view').naturalHeight")
test("Image loaded (naturalWidth > 0)", nat_w > 0, f"{nat_w}x{nat_h}")

page.screenshot(path=f"{SCREENSHOT_DIR}/02_tcp_stream.png")
print()

# ── 3. HUD Display ──
print("── 3. HUD Overlay ──")
time.sleep(2)  # let status update
fps_text = page.locator("#hud-fps").inner_text()
temp_text = page.locator("#hud-temp").inner_text()
test("FPS HUD visible", page.locator("#hud-fps").is_visible())
test("FPS shows value", "FPS:" in fps_text and fps_text != "FPS: --", fps_text)
test("Temp HUD visible", page.locator("#hud-temp").is_visible())
test("Temp shows value", "TEMP:" in temp_text and temp_text != "TEMP: --°C", temp_text)
print()

# ── 4. System Info Panel ──
print("── 4. System Info Panel ──")
si_uptime = page.locator("#si-uptime").inner_text()
si_heap = page.locator("#si-heap").inner_text()
si_rssi = page.locator("#si-rssi").inner_text()
si_wifi = page.locator("#si-wifi").inner_text()
si_temp = page.locator("#si-temp").inner_text()
fw_ver = page.locator("#fw-ver").inner_text()
test("Uptime displayed", si_uptime != "--", si_uptime)
test("Heap displayed", "KB" in si_heap, si_heap)
test("RSSI displayed", "dBm" in si_rssi, si_rssi)
test("WiFi status", si_wifi == "Connected", si_wifi)
test("Temperature in info", "°C" in si_temp, si_temp)
test("Firmware version shown", "v1.1.0" in fw_ver, fw_ver)

page.screenshot(path=f"{SCREENSHOT_DIR}/03_system_info.png")
print()

# ── 5. LED Toggle ──
print("── 5. LED Control ──")
led_btn = page.locator("#led-btn")
test("LED button visible", led_btn.is_visible())
initial_text = led_btn.inner_text()
test("LED starts OFF", "OFF" in initial_text, initial_text)

# Click to turn ON
led_btn.click()
time.sleep(1.5)
after_on_text = led_btn.inner_text()
test("LED toggles to ON", "ON" in after_on_text, after_on_text)
test("LED button has 'on' class", "on" in led_btn.get_attribute("class"))

page.screenshot(path=f"{SCREENSHOT_DIR}/04_led_on.png")

# Click to turn OFF
led_btn.click()
time.sleep(1.5)
after_off_text = led_btn.inner_text()
test("LED toggles back to OFF", "OFF" in after_off_text, after_off_text)

page.screenshot(path=f"{SCREENSHOT_DIR}/05_led_off.png")
print()

# ── 6. Snapshot Button ──
print("── 6. Snapshot ──")
snap_btn = page.locator("button.snap")
test("Snapshot button visible", snap_btn.is_visible())

# Click — triggers a download
with page.expect_download(timeout=5000) as download_info:
    snap_btn.click()
download = download_info.value
snap_path = download.path()
test("Download triggered", snap_path is not None)
if snap_path:
    snap_size = os.path.getsize(snap_path)
    test("Downloaded file > 5KB", snap_size > 5000, f"{snap_size} bytes")
    # Copy to screenshot dir for review
    download.save_as(f"{SCREENSHOT_DIR}/06_snapshot.jpg")
print()

# ── 7. WS Stream Tab ──
print("── 7. WebSocket Stream ──")
ws_tab.click()
time.sleep(3)

ws_canvas = page.locator("#ws-canvas")
test("WS canvas visible", ws_canvas.is_visible())
test("TCP view hidden", not page.locator("#tcp-view").is_visible())

# Check WS status
ws_hud = page.locator("#hud-ws")
test("WS HUD visible", ws_hud.is_visible())
ws_text = ws_hud.inner_text()
test("WS connected", "connected" in ws_text.lower(), ws_text)

# Wait for frames
time.sleep(3)
# Check canvas has content (non-zero pixel data)
has_content = page.evaluate("""() => {
    const c = document.getElementById('ws-canvas');
    const ctx = c.getContext('2d');
    const data = ctx.getImageData(Math.floor(c.width/2), Math.floor(c.height/2), 1, 1).data;
    return data[0] + data[1] + data[2] > 0;
}""")
test("Canvas has rendered content", has_content)

# Check WS FPS in HUD
time.sleep(2)
fps_text_ws = page.locator("#hud-fps").inner_text()
test("WS FPS shows value", "FPS:" in fps_text_ws and fps_text_ws != "FPS: --", fps_text_ws)

# Quality selector visible in WS mode
q_sel = page.locator("#sel-q")
test("Quality selector visible in WS mode", q_sel.is_visible())
r_sel = page.locator("#sel-res")
test("Resolution selector visible in WS mode", r_sel.is_visible())

page.screenshot(path=f"{SCREENSHOT_DIR}/07_ws_stream.png")
print()

# ── 8. WS Quality/Resolution Controls ──
print("── 8. WS Controls ──")
# Change quality
q_sel.select_option("20")
time.sleep(1)
test("Quality changed to Q:20", q_sel.input_value() == "20")

# Change resolution
r_sel.select_option("SVGA")
time.sleep(2)
test("Resolution changed to SVGA", r_sel.input_value() == "SVGA")

# Verify canvas still receiving
time.sleep(2)
has_content2 = page.evaluate("""() => {
    const c = document.getElementById('ws-canvas');
    const ctx = c.getContext('2d');
    const data = ctx.getImageData(Math.floor(c.width/2), Math.floor(c.height/2), 1, 1).data;
    return data[0] + data[1] + data[2] > 0;
}""")
test("Canvas still rendering after setting change", has_content2)

# Reset to defaults
q_sel.select_option("12")
r_sel.select_option("VGA")
time.sleep(1)

page.screenshot(path=f"{SCREENSHOT_DIR}/08_ws_controls.png")
print()

# ── 9. OTA Panel ──
print("── 9. OTA Panel ──")
# Open OTA details panel
ota_details = page.locator("details").nth(1)
ota_details.click()
time.sleep(0.5)

ota_input = page.locator("#ota-url")
ota_btn = page.locator("#ota-btn")
ota_status = page.locator("#ota-status")
test("OTA URL input visible", ota_input.is_visible())
test("OTA button visible", ota_btn.is_visible())
test("OTA status shows Ready", "Ready" in ota_status.inner_text())

page.screenshot(path=f"{SCREENSHOT_DIR}/09_ota_panel.png")
print()

# ── 10. Switch back to TCP ──
print("── 10. Tab Switching ──")
tcp_tab.click()
time.sleep(2)
test("TCP view visible after switch back", page.locator("#tcp-view").is_visible())
test("WS canvas hidden after switch back", not page.locator("#ws-canvas").is_visible())
# TCP stream still works
time.sleep(2)
nat_w2 = page.evaluate("document.getElementById('tcp-view').naturalWidth")
test("TCP stream resumes", nat_w2 > 0, f"width={nat_w2}")

page.screenshot(path=f"{SCREENSHOT_DIR}/10_tcp_back.png")
print()

# ── 11. Status bar updates ──
print("── 11. Status Bar ──")
status_bar = page.locator("#status").inner_text()
test("Status bar shows connected", "Connected" in status_bar, status_bar)
print()

# ── 12. Standalone WS page (/stream/ws) ──
print("── 12. Standalone WS Page ──")
page.goto(f"{BASE}/stream/ws", wait_until="networkidle", timeout=15000)
time.sleep(3)

test("WS page title", "WebSocket" in page.title())
ws_status = page.locator("#hud-ws").inner_text()
test("WS connected on standalone page", "connected" in ws_status.lower(), ws_status)

# Check canvas rendering
time.sleep(3)
has_content3 = page.evaluate("""() => {
    const c = document.getElementById('canvas');
    const ctx = c.getContext('2d');
    const data = ctx.getImageData(Math.floor(c.width/2), Math.floor(c.height/2), 1, 1).data;
    return data[0] + data[1] + data[2] > 0;
}""")
test("Standalone WS canvas rendering", has_content3)

# Navigation link back
nav_link = page.locator(".nav a")
test("Back link to / exists", nav_link.is_visible())
link_href = nav_link.get_attribute("href")
test("Link points to /", link_href == "/", link_href)

page.screenshot(path=f"{SCREENSHOT_DIR}/11_standalone_ws.png")
print()

# Navigate back to dashboard
page.goto(f"{BASE}/", wait_until="networkidle", timeout=10000)

# ── Summary ──
print()
print("=" * 55)
print(f" BROWSER QA SUMMARY")
print(f" Passed: {passed}")
print(f" Failed: {failed}")
print(f" Total:  {passed + failed}")
print(f" Screenshots: {SCREENSHOT_DIR}/")
print("=" * 55)

if failed > 0:
    print("\n FAILED tests:")
    for status, name, detail in results:
        if status == "FAIL":
            print(f"  ❌ {name}: {detail}")

# Keep browser open for manual inspection
print("\nBrowser left open for inspection. Press Ctrl+C to close.")
import signal
signal.signal(signal.SIGINT, lambda *_: None)
signal.signal(signal.SIGTERM, lambda *_: None)

sys.exit(1 if failed > 0 else 0)
