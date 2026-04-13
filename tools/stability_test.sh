#!/bin/bash
# Multi-client stability test
IP=192.168.1.171

echo "=== Multi-Client Stability Test ==="

# 1. Start 1 MJPEG stream consumer (port 8081)
echo "[1] Starting MJPEG consumer..."
curl -s --max-time 60 -o /dev/null "http://$IP:8081/stream/tcp" &
MJPEG_PID=$!

# 2. Start 3 API pollers (simulating concurrent clients)
echo "[2] Starting 3 API pollers..."
for j in 1 2 3; do
    (for i in $(seq 1 30); do curl -s "http://$IP/api/status" > /dev/null; sleep 2; done) &
done

# 3. Monitor heap over 60 seconds
echo "[3] Monitoring heap for 60s..."
heap_start=""
for i in $(seq 1 12); do
    sleep 5
    st=$(curl -s --connect-timeout 3 "http://$IP/api/status" 2>/dev/null)
    if [ -n "$st" ]; then
        heap=$(echo "$st" | python3 -c "import sys,json; print(json.load(sys.stdin).get('heap_free',0))" 2>/dev/null)
        fps=$(echo "$st" | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'{d.get(\"fps\",0):.1f}')" 2>/dev/null)
        uptime=$(echo "$st" | python3 -c "import sys,json; print(json.load(sys.stdin).get('uptime',0))" 2>/dev/null)
        echo "  [$i] uptime=${uptime}s heap=${heap} fps=${fps}"
        if [ -z "$heap_start" ]; then heap_start=$heap; fi
        heap_end=$heap
    else
        echo "  [$i] UNREACHABLE"
    fi
done

# 4. Kill background processes
kill $MJPEG_PID 2>/dev/null
wait 2>/dev/null

# 5. Report
echo ""
echo "=== Stability Report ==="
echo "  Heap start: $heap_start"
echo "  Heap end:   $heap_end"
if [ -n "$heap_start" ] && [ -n "$heap_end" ]; then
    diff=$((heap_end - heap_start))
    echo "  Heap drift: $diff bytes"
    if [ $diff -gt -50000 ] && [ $diff -lt 50000 ]; then
        echo "  RESULT: STABLE (drift < 50KB)"
    else
        echo "  RESULT: LEAK SUSPECTED"
    fi
fi
