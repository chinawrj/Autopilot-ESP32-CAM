#!/usr/bin/env python3
"""Take screenshots of ESP32-CAM web pages for release documentation."""
import os
import sys
import time

from patchright.sync_api import sync_playwright

DEVICE_IP = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.171"
USER_DATA_DIR = os.path.expanduser("~/.patchright-userdata/esp32cam-screenshots")
SCREENSHOT_DIR = os.path.join(os.path.dirname(__file__), "..", "docs", "images")
os.makedirs(SCREENSHOT_DIR, exist_ok=True)

pw = sync_playwright().start()
context = pw.chromium.launch_persistent_context(
    user_data_dir=USER_DATA_DIR,
    channel="chrome",
    headless=False,
    no_viewport=True,
    ignore_default_args=["--no-sandbox"],
    viewport={"width": 800, "height": 700},
)

def take_screenshot(context, url, path, wait_secs=5, action=None):
    """Open a new tab, take screenshot, close tab."""
    page = context.new_page()
    try:
        page.goto(url, wait_until="domcontentloaded", timeout=30000)
        time.sleep(wait_secs)
        if action:
            action(page)
        page.screenshot(path=path)
        print(f"  Saved: {path}")
    finally:
        page.close()

try:
    # Screenshot 1: MJPEG stream page
    print(f"[1/4] Opening MJPEG page: http://{DEVICE_IP}/")
    path1 = os.path.join(SCREENSHOT_DIR, "mjpeg-stream.png")
    take_screenshot(context, f"http://{DEVICE_IP}/", path1, wait_secs=5)

    # Screenshot 2: WebSocket stream page
    print(f"[2/4] Opening WebSocket page: http://{DEVICE_IP}/stream/udp")
    path2 = os.path.join(SCREENSHOT_DIR, "ws-stream.png")
    take_screenshot(context, f"http://{DEVICE_IP}/stream/udp", path2, wait_secs=5)

    # Screenshot 3: LED ON state
    print("[3/4] Toggling LED ON for screenshot...")
    path3 = os.path.join(SCREENSHOT_DIR, "led-on.png")
    def toggle_led(page):
        page.click("#led-btn")
        time.sleep(2)
    take_screenshot(context, f"http://{DEVICE_IP}/", path3, wait_secs=3, action=toggle_led)
    # Toggle LED back off
    import urllib.request
    urllib.request.urlopen(urllib.request.Request(
        f"http://{DEVICE_IP}/api/led",
        data=b'{"state":"off"}',
        headers={"Content-Type": "application/json"},
        method="POST"
    ))

    # Screenshot 4: API status response
    print("[4/4] Capturing API status...")
    path4 = os.path.join(SCREENSHOT_DIR, "api-status.png")
    take_screenshot(context, f"http://{DEVICE_IP}/api/status", path4, wait_secs=1)

    print("\nAll screenshots saved to docs/images/")
    print("Done!")

finally:
    context.close()
    pw.stop()
