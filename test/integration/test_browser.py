#!/usr/bin/env python3
"""
Autopilot ESP32-CAM — Browser Integration Tests

Requirements:
    pip install patchright
    patchright install chromium

Usage:
    python3 test/integration/test_browser.py [--device IP] [--headless]

Verifies all HTTP endpoints, WebSocket page, security headers, and
path traversal protection using a real Chrome browser via Patchright.
"""

import argparse
import json
import sys
import time

from patchright.sync_api import sync_playwright

DEFAULT_DEVICE = "http://192.168.1.171"
USER_DATA_DIR = "/Users/rjwang/.patchright-userdata"


class TestRunner:
    def __init__(self):
        self.results = []

    def test(self, name, passed, detail=""):
        status = "PASS" if passed else "FAIL"
        self.results.append((name, passed))
        msg = f"  [{status}] {name}"
        if detail:
            msg += f" — {detail}"
        print(msg)

    @property
    def passed(self):
        return sum(1 for _, p in self.results if p)

    @property
    def total(self):
        return len(self.results)

    def summary(self):
        print(f"\n{'=' * 50}")
        print(f"Results: {self.passed}/{self.total} passed")
        if self.passed < self.total:
            print("FAILED:")
            for name, p in self.results:
                if not p:
                    print(f"  ✗ {name}")
        return self.passed == self.total


def run_tests(device_url, headless=False):
    t = TestRunner()

    with sync_playwright() as p:
        browser = p.chromium.launch_persistent_context(
            user_data_dir=USER_DATA_DIR,
            channel="chrome",
            headless=headless,
            args=["--window-size=1200,900"],
        )
        page = browser.new_page()

        # --- Homepage ---
        page.goto(f"{device_url}/", timeout=8000)
        title = page.title()
        t.test("Homepage loads", "ESP32" in title or "Autopilot" in title, title)
        time.sleep(0.5)

        # --- Status API ---
        page.goto(f"{device_url}/api/status", timeout=5000)
        data = json.loads(page.inner_text("body"))
        t.test("Status API", "fps" in data and "heap_free" in data)
        time.sleep(0.3)

        # --- System info API ---
        page.goto(f"{device_url}/api/system/info", timeout=5000)
        info = json.loads(page.inner_text("body"))
        t.test("System info - chip", info.get("chip", {}).get("model") == "ESP32")
        t.test("System info - IDF", "v5." in info.get("idf_version", ""))
        t.test(
            "System info - PSRAM",
            info.get("memory", {}).get("psram_total", 0) > 0,
            f"{info.get('memory', {}).get('psram_total', 0) // 1024}KB",
        )
        t.test("System info - uptime", info.get("uptime_s", 0) > 0, info.get("uptime"))
        t.test(
            "System info - tasks",
            info.get("task_count", 0) > 5,
            f"{info.get('task_count')} tasks",
        )
        t.test("System info - WiFi", info.get("wifi_connected") is True)
        time.sleep(0.3)

        # --- Security headers ---
        hdrs = page.evaluate(
            """async () => {
            const r = await fetch('/api/status');
            return {
                xcto: r.headers.get('X-Content-Type-Options'),
                xfo: r.headers.get('X-Frame-Options'),
                cc: r.headers.get('Cache-Control')
            };
        }"""
        )
        t.test("Header: X-Content-Type-Options", hdrs.get("xcto") == "nosniff")
        t.test("Header: X-Frame-Options", hdrs.get("xfo") == "SAMEORIGIN")
        t.test("Header: Cache-Control", hdrs.get("cc") == "no-store")
        time.sleep(0.3)

        # --- LED toggle ---
        led = page.evaluate(
            """async () => {
            const r = await fetch('/api/led', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({state: 'toggle'})
            });
            return await r.json();
        }"""
        )
        t.test("LED toggle", "led_state" in led)
        time.sleep(0.3)

        # --- Snapshot ---
        page.goto(f"{device_url}/api/snapshot", timeout=8000)
        ct = page.evaluate("() => document.contentType")
        t.test("Snapshot JPEG", ct == "image/jpeg")
        time.sleep(0.3)

        # --- Camera settings ---
        page.goto(f"{device_url}/api/camera", timeout=5000)
        cam = json.loads(page.inner_text("body"))
        t.test("Camera settings", "framesize" in cam)
        time.sleep(0.3)

        # --- Path traversal blocked ---
        page.goto(f"{device_url}/api/sd/list?path=../../etc", timeout=5000)
        t.test("Path traversal blocked", "Invalid path" in page.inner_text("body"))
        time.sleep(0.3)

        # --- WebSocket stream page ---
        page.goto(f"{device_url}/stream/ws", timeout=8000)
        ws_title = page.title()
        t.test("WS stream page", len(ws_title) > 0, ws_title)

        browser.close()

    return t.summary()


def main():
    parser = argparse.ArgumentParser(description="ESP32-CAM Browser Integration Tests")
    parser.add_argument("--device", default=DEFAULT_DEVICE, help="Device URL")
    parser.add_argument("--headless", action="store_true", help="Run headless")
    args = parser.parse_args()

    print(f"Testing device: {args.device}")
    print(f"Mode: {'headless' if args.headless else 'visible'}")
    print("-" * 50)

    success = run_tests(args.device, args.headless)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
