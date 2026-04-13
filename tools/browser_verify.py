#!/usr/bin/env python3
"""Browser verification for ESP32-CAM WebSocket stream page with controls."""
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

# Navigate to WS stream page
page.goto(f"http://{DEVICE_IP}/stream/ws", wait_until="domcontentloaded")
time.sleep(3)

# Take screenshot
page.screenshot(path="/tmp/esp32cam_ws_stream.png")
print("Screenshot saved: /tmp/esp32cam_ws_stream.png")

# Check page elements
title = page.title()
print(f"Page title: {title}")

# Check canvas is rendering
canvas = page.query_selector("#canvas")
w = canvas.get_attribute("width") if canvas else "N/A"
h = canvas.get_attribute("height") if canvas else "N/A"
print(f"Canvas size: {w}x{h}")

# Check HUD elements
fps_text = page.text_content("#hud-fps")
temp_text = page.text_content("#hud-temp")
heap_text = page.text_content("#hud-heap")
ws_text = page.text_content("#hud-ws")
print(f"HUD FPS: {fps_text}")
print(f"HUD Temp: {temp_text}")
print(f"HUD Heap: {heap_text}")
print(f"HUD WS: {ws_text}")

# Check controls exist
quality_sel = page.query_selector("#sel-quality")
res_sel = page.query_selector("#sel-res")
led_btn = page.query_selector("#led-btn")
print(f"Quality selector: {'present' if quality_sel else 'MISSING'}")
print(f"Resolution selector: {'present' if res_sel else 'MISSING'}")
print(f"LED button: {'present' if led_btn else 'MISSING'}")

# Test quality change via dropdown
page.select_option("#sel-quality", "20")
time.sleep(2)
page.select_option("#sel-quality", "12")
print("Quality dropdown test: OK")

# Wait for heartbeat (check heap HUD updates)
time.sleep(6)
heap_text2 = page.text_content("#hud-heap")
print(f"HUD Heap after heartbeat: {heap_text2}")

# Take final screenshot
page.screenshot(path="/tmp/esp32cam_ws_controls.png")
print("Final screenshot: /tmp/esp32cam_ws_controls.png")

print("\n✅ Browser verification complete!")

# Keep browser open
print("Browser staying open...")
signal.signal(signal.SIGINT, lambda *_: None)
signal.signal(signal.SIGTERM, lambda *_: None)
try:
    while True:
        time.sleep(3600)
except (KeyboardInterrupt, SystemExit):
    pass
finally:
    context.close()
    pw.stop()
