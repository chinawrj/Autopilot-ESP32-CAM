#!/usr/bin/env python3
"""M5: Heap trend monitor — polls /api/status every 10s for N minutes.
Also maintains a WS stream connection to simulate real usage.
Usage: python3 tools/heap_monitor.py [minutes] [device_ip]
"""
import asyncio, json, sys, time, websockets

MINUTES = int(sys.argv[1]) if len(sys.argv) > 1 else 10
DEVICE_IP = sys.argv[2] if len(sys.argv) > 2 else "192.168.1.171"
INTERVAL = 10

async def ws_consumer(uri, stop_event):
    """Keep a WS stream connection open, consuming frames."""
    frames = 0
    while not stop_event.is_set():
        try:
            async with websockets.connect(uri) as ws:
                while not stop_event.is_set():
                    await asyncio.wait_for(ws.recv(), timeout=5)
                    frames += 1
        except Exception:
            await asyncio.sleep(2)
    return frames

async def main():
    import aiohttp
    url = f"http://{DEVICE_IP}/api/status"
    ws_uri = f"ws://{DEVICE_IP}/ws/stream"
    stop = asyncio.Event()

    ws_task = asyncio.create_task(ws_consumer(ws_uri, stop))

    samples = []
    start = time.time()
    duration = MINUTES * 60
    print(f"Monitoring heap for {MINUTES} min (poll every {INTERVAL}s) — device {DEVICE_IP}")
    print(f"{'Time':>6} {'heap_free':>12} {'heap_min':>12} {'delta':>8} {'fps':>6} {'temp':>6}")
    print("-" * 60)

    async with aiohttp.ClientSession() as session:
        prev_heap = None
        while time.time() - start < duration:
            try:
                async with session.get(url, timeout=aiohttp.ClientTimeout(total=5)) as resp:
                    data = await resp.json()
                heap = data["heap_free"]
                heap_min = data["heap_min"]
                fps = data["fps"]
                temp = data["temperature"]
                elapsed = int(time.time() - start)
                delta = heap - prev_heap if prev_heap is not None else 0
                print(f"{elapsed:>5}s {heap:>12} {heap_min:>12} {delta:>+8} {fps:>6.1f} {temp:>5.1f}°C")
                samples.append({"t": elapsed, "heap_free": heap, "heap_min": heap_min})
                prev_heap = heap
            except Exception as e:
                print(f"  [poll error: {e}]")
            await asyncio.sleep(INTERVAL)

    stop.set()
    frames = await ws_task

    # Summary
    if len(samples) >= 2:
        first = samples[0]["heap_free"]
        last = samples[-1]["heap_free"]
        low = min(s["heap_min"] for s in samples)
        drift = last - first
        drift_per_min = drift / (samples[-1]["t"] / 60) if samples[-1]["t"] > 0 else 0
        print(f"\n{'='*60}")
        print(f"Duration:     {samples[-1]['t']}s ({MINUTES} min)")
        print(f"WS frames:    {frames}")
        print(f"Heap start:   {first} bytes ({first//1024} KB)")
        print(f"Heap end:     {last} bytes ({last//1024} KB)")
        print(f"Heap drift:   {drift:+d} bytes ({drift_per_min:+.0f} bytes/min)")
        print(f"Heap minimum: {low} bytes ({low//1024} KB)")
        if abs(drift_per_min) < 500:
            print("✅ No significant heap leak detected")
        else:
            print(f"⚠️  Heap drift {drift_per_min:+.0f} bytes/min — investigate!")
    else:
        print("Not enough samples")

asyncio.run(main())
