#!/bin/bash
# OTA E2E test with IPv4-only HTTP server
set -e

LOCAL_IP=192.168.1.141
DEVICE_IP=192.168.1.171
PORT=8070
FW_DIR=/Users/rjwang/fun/Autopilot-ESP32-CAM/build

# Kill any leftover server
lsof -ti :$PORT | xargs kill -9 2>/dev/null || true
sleep 1

# Start IPv4-only HTTP server
cd "$FW_DIR"
python3 -c "
import http.server, socketserver
h = http.server.SimpleHTTPRequestHandler
with socketserver.TCPServer(('0.0.0.0', $PORT), h) as s:
    print('Serving on 0.0.0.0:$PORT')
    s.serve_forever()
" &
PY_PID=$!
sleep 1

# Verify server works
code=$(curl -s -o /dev/null -w "%{http_code}" "http://$LOCAL_IP:$PORT/autopilot-esp32-cam.bin")
echo "Local test: HTTP $code"
if [ "$code" != "200" ]; then
    echo "ERROR: Server not accessible"
    kill $PY_PID 2>/dev/null
    exit 1
fi

# Trigger OTA
echo "Triggering OTA from $DEVICE_IP..."
curl -s -X POST -H "Content-Type: application/json" \
    -d "{\"url\":\"http://$LOCAL_IP:$PORT/autopilot-esp32-cam.bin\"}" \
    "http://$DEVICE_IP/api/ota"
echo ""

# Monitor progress
PASS=0
for i in $(seq 1 50); do
    sleep 5
    resp=$(curl -s --connect-timeout 3 "http://$DEVICE_IP/api/ota/status" 2>/dev/null)
    if [ -n "$resp" ]; then
        ota_st=$(echo "$resp" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('status','?'))" 2>/dev/null)
        ota_pct=$(echo "$resp" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('progress',0))" 2>/dev/null)
        echo "[$i] status=$ota_st progress=$ota_pct%"

        if [ "$ota_st" = "idle" ] && [ $i -gt 5 ]; then
            echo "=== OTA COMPLETE - device rebooted ==="
            echo "Post-OTA health:"
            curl -s "http://$DEVICE_IP/api/status" | python3 -m json.tool
            PASS=1
            break
        fi

        if echo "$ota_st" | grep -q "error"; then
            echo "=== OTA FAILED: $ota_st ==="
            break
        fi
    else
        echo "[$i] unreachable (rebooting?)"
    fi
done

kill $PY_PID 2>/dev/null
echo "HTTP server stopped"

if [ $PASS -eq 1 ]; then
    echo "OTA E2E TEST: PASSED"
else
    echo "OTA E2E TEST: FAILED"
fi
