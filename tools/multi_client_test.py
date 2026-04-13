#!/usr/bin/env python3
"""M5: Multi-client concurrent stress test.
Opens N WebSocket streams + 1 MJPEG stream simultaneously.
Usage: python3 tools/multi_client_test.py [ws_clients] [duration_s] [device_ip]
"""
import asyncio, json, sys, time, websockets, aiohttp

WS_CLIENTS = int(sys.argv[1]) if len(sys.argv) > 1 else 3
DURATION = int(sys.argv[2]) if len(sys.argv) > 2 else 60
DEVICE_IP = sys.argv[3] if len(sys.argv) > 3 else "192.168.1.171"

async def ws_client(client_id, uri, stats, stop):
    """Single WS stream consumer."""
    while not stop.is_set():
        try:
            async with websockets.connect(uri) as ws:
                while not stop.is_set():
                    data = await asyncio.wait_for(ws.recv(), timeout=5)
                    if isinstance(data, bytes):
                        stats[client_id]["frames"] += 1
                        stats[client_id]["bytes"] += len(data)
        except asyncio.TimeoutError:
            stats[client_id]["timeouts"] += 1
        except Exception as e:
            stats[client_id]["errors"] += 1
            await asyncio.sleep(1)

async def mjpeg_client(url, stats, stop):
    """MJPEG stream consumer (TCP path)."""
    key = "mjpeg"
    try:
        async with aiohttp.ClientSession() as session:
            async with session.get(url, timeout=aiohttp.ClientTimeout(total=DURATION+10)) as resp:
                boundary = b"--frame"
                buf = b""
                async for chunk in resp.content.iter_any():
                    if stop.is_set():
                        break
                    buf += chunk
                    while boundary in buf:
                        idx = buf.index(boundary)
                        frame = buf[:idx]
                        buf = buf[idx + len(boundary):]
                        if b"\xff\xd8" in frame and b"\xff\xd9" in frame:
                            stats[key]["frames"] += 1
                            stats[key]["bytes"] += len(frame)
    except Exception as e:
        stats[key]["errors"] += 1

async def main():
    ws_uri = f"ws://{DEVICE_IP}/ws/stream"
    mjpeg_url = f"http://{DEVICE_IP}/stream/tcp"
    status_url = f"http://{DEVICE_IP}/api/status"
    stop = asyncio.Event()

    stats = {}
    for i in range(WS_CLIENTS):
        stats[f"ws{i}"] = {"frames": 0, "bytes": 0, "errors": 0, "timeouts": 0}
    stats["mjpeg"] = {"frames": 0, "bytes": 0, "errors": 0, "timeouts": 0}

    tasks = []
    for i in range(WS_CLIENTS):
        tasks.append(asyncio.create_task(ws_client(f"ws{i}", ws_uri, stats, stop)))
    tasks.append(asyncio.create_task(mjpeg_client(mjpeg_url, stats, stop)))

    print(f"Running {WS_CLIENTS} WS + 1 MJPEG clients for {DURATION}s — {DEVICE_IP}")
    print("-" * 70)

    start = time.time()
    while time.time() - start < DURATION:
        await asyncio.sleep(10)
        elapsed = int(time.time() - start)
        try:
            async with aiohttp.ClientSession() as session:
                async with session.get(status_url, timeout=aiohttp.ClientTimeout(total=3)) as resp:
                    d = await resp.json()
            total_ws_frames = sum(stats[f"ws{i}"]["frames"] for i in range(WS_CLIENTS))
            total_ws_errs = sum(stats[f"ws{i}"]["errors"] for i in range(WS_CLIENTS))
            print(f"  {elapsed:>4}s | WS frames: {total_ws_frames:>6} errs: {total_ws_errs}"
                  f" | MJPEG: {stats['mjpeg']['frames']:>5}"
                  f" | heap: {d['heap_free']//1024}KB min:{d['heap_min']//1024}KB")
        except Exception as e:
            print(f"  {elapsed:>4}s | poll error: {e}")

    stop.set()
    await asyncio.sleep(2)
    for t in tasks:
        t.cancel()
    await asyncio.gather(*tasks, return_exceptions=True)

    # Final summary
    print(f"\n{'='*70}")
    print(f"Duration: {DURATION}s | WS clients: {WS_CLIENTS} + 1 MJPEG")
    for key, s in sorted(stats.items()):
        fps = s["frames"] / DURATION if DURATION > 0 else 0
        print(f"  {key:>6}: {s['frames']:>6} frames ({fps:.1f} fps), "
              f"{s['bytes']//1024:>6} KB, {s['errors']} errors, {s['timeouts']} timeouts")

    try:
        async with aiohttp.ClientSession() as session:
            async with session.get(status_url, timeout=aiohttp.ClientTimeout(total=3)) as resp:
                d = await resp.json()
        print(f"  Final heap: {d['heap_free']//1024}KB (min: {d['heap_min']//1024}KB)")
    except:
        pass

    total_errs = sum(s["errors"] for s in stats.values())
    if total_errs == 0:
        print("✅ Multi-client test passed — no errors!")
    else:
        print(f"⚠️  {total_errs} total errors detected")

asyncio.run(main())
