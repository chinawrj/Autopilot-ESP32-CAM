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
    if not device_url.startswith("http"):
        device_url = f"http://{device_url}"
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
        time.sleep(1)

        # --- app.js external script ---
        js_resp = page.evaluate(
            """async () => {
            const r = await fetch('/app.js');
            const ct = r.headers.get('Content-Type');
            const body = await r.text();
            return {status: r.status, ct: ct, len: body.length, hasSwitch: body.includes('switchMode')};
        }"""
        )
        t.test("app.js served", js_resp.get("status") == 200 and js_resp.get("len") > 1000)
        t.test("app.js content-type", "javascript" in (js_resp.get("ct") or ""))
        t.test("app.js has functions", js_resp.get("hasSwitch") is True)
        time.sleep(0.3)

        # --- Homepage System Info panel (populated after JS fetch) ---
        try:
            page.wait_for_function(
                """() => {
                    const el = document.getElementById('si-chip');
                    return el && el.textContent && el.textContent !== '--';
                }""",
                timeout=15000,
            )
            panel_ready = True
        except Exception:
            panel_ready = False
        chip_text = page.inner_text("#si-chip")
        t.test("Panel: chip populated", "ESP32" in chip_text, chip_text)
        idf_text = page.inner_text("#si-idf")
        t.test("Panel: IDF populated", "v5." in idf_text, idf_text)
        psram_text = page.inner_text("#si-psram")
        t.test("Panel: PSRAM populated", "KB" in psram_text, psram_text)
        tasks_text = page.inner_text("#si-tasks")
        t.test("Panel: tasks populated", tasks_text.isdigit() and int(tasks_text) > 5, tasks_text)
        time.sleep(0.3)

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
        health = info.get("health", {})
        t.test("Health - heap OK", health.get("heap_ok") is True)
        t.test("Health - memory OK", health.get("memory_ok") is True)
        t.test("Health - watchdog", health.get("wdt_timeout_s", 0) >= 10,
               f"{health.get('wdt_timeout_s')}s")
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
        time.sleep(0.3)

        # --- OTA status ---
        ota = page.evaluate(
            """async () => {
            const r = await fetch('/api/ota/status');
            return await r.json();
        }"""
        )
        t.test("OTA status", ota.get("status") == "idle" and "version" in ota,
               f"status={ota.get('status')}")
        time.sleep(0.3)

        # --- SD card status ---
        sd = page.evaluate(
            """async () => {
            const r = await fetch('/api/sd/status');
            return await r.json();
        }"""
        )
        t.test("SD status", "mounted" in sd, f"mounted={sd.get('mounted')}")
        time.sleep(0.3)

        # --- SD list (root) ---
        sd_list = page.evaluate(
            """async () => {
            const r = await fetch('/api/sd/list');
            return await r.json();
        }"""
        )
        t.test("SD list", "mounted" in sd_list or "error" in sd_list)
        time.sleep(0.3)

        # --- MJPEG stream (check content-type via curl, MJPEG is infinite stream) ---
        # Navigate away from homepage first to release the browser's MJPEG connection
        page.goto("about:blank", timeout=3000)
        time.sleep(2)
        import subprocess
        host = device_url.replace("http://", "").replace("https://", "")
        curl_result = subprocess.run(
            ["curl", "-s", "-o", "/dev/null", "-w", "%{content_type}",
             "--max-time", "2", f"http://{host}:8081/stream/tcp"],
            capture_output=True, text=True
        )
        curl_ct = curl_result.stdout.strip()
        t.test("MJPEG stream headers", curl_ct.startswith("multipart/"), curl_ct)
        # Navigate back to device for remaining tests
        page.goto(f"{device_url}/", timeout=8000)
        time.sleep(1)

        # --- Camera POST (set and restore quality) ---
        cam_set = page.evaluate(
            """async () => {
            const r = await fetch('/api/camera', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({quality: 20})
            });
            return await r.json();
        }"""
        )
        t.test("Camera POST", cam_set.get("quality") == 20, f"quality={cam_set.get('quality')}")
        # Restore default quality
        page.evaluate(
            """async () => {
            await fetch('/api/camera', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({quality: 12})
            });
        }"""
        )
        time.sleep(0.3)

        # --- LED on/off cycle ---
        led_on = page.evaluate(
            """async () => {
            const r = await fetch('/api/led', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({state: 'on'})
            });
            return await r.json();
        }"""
        )
        t.test("LED on", led_on.get("led_state") is True)
        led_off = page.evaluate(
            """async () => {
            const r = await fetch('/api/led', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({state: 'off'})
            });
            return await r.json();
        }"""
        )
        t.test("LED off", led_off.get("led_state") is False)
        time.sleep(0.3)

        # --- LED bad request ---
        led_bad = page.evaluate(
            """async () => {
            const r = await fetch('/api/led', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: 'not json'
            });
            return {status: r.status};
        }"""
        )
        t.test("LED bad JSON", led_bad.get("status") == 400)
        time.sleep(0.3)

        # --- 404 for unknown route ---
        resp_404 = page.evaluate(
            """async () => {
            const r = await fetch('/api/nonexistent');
            return {status: r.status};
        }"""
        )
        t.test("404 unknown route", resp_404.get("status") == 404)
        time.sleep(0.3)

        # --- Status API has version ---
        status2 = page.evaluate(
            """async () => {
            const r = await fetch('/api/status');
            return await r.json();
        }"""
        )
        t.test("Status has version", "1." in str(status2.get("version", "")),
               status2.get("version"))
        t.test("Status has WiFi", status2.get("wifi_connected") is True)
        t.test("Status has RSSI", isinstance(status2.get("rssi"), (int, float)) and status2["rssi"] < 0,
               f"{status2.get('rssi')}dBm")
        time.sleep(0.3)

        # --- CORS header ---
        cors = page.evaluate(
            """async () => {
            const r = await fetch('/api/status');
            return r.headers.get('Access-Control-Allow-Origin');
        }"""
        )
        t.test("CORS header", cors == "*")

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
