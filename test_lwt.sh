#!/bin/bash
# Test LWT behavior in different scenarios

echo "=== LWT Testing Script ==="
echo ""

# Start subscriber in background
echo "Starting subscriber to watch status messages..."
mosquitto_sub -h localhost -t 'devices/+/status' -v > lwt_test.log &
SUBSCRIBER_PID=$!
sleep 1

echo ""
echo "=== Test 1: Graceful Shutdown (Ctrl+C) ==="
echo "Starting device agent..."
timeout 5 ../bin/device_agent graceful-test
echo "✓ Check lwt_test.log - Should see 'online' then 'offline'"
echo "  LWT should NOT fire (graceful disconnect)"
sleep 2

echo ""
echo "=== Test 2: Ungraceful Shutdown (kill -9) ==="
echo "Starting device agent..."
../bin/device_agent crash-test &
DEVICE_PID=$!
sleep 3
echo "Killing process abruptly with kill -9..."
kill -9 $DEVICE_PID
sleep 2
echo "✓ Check lwt_test.log - Should see 'online' then LWT 'offline'"
echo "  LWT SHOULD fire (ungraceful disconnect)"
sleep 2

echo ""
echo "=== Test 3: Retained Message Behavior ==="
echo "Starting new subscriber AFTER device died..."
timeout 3 mosquitto_sub -h localhost -t 'devices/crash-test/status' -v
echo "✓ You should immediately see the 'offline' status"
echo "  This proves the retained flag works!"

# Cleanup
kill $SUBSCRIBER_PID 2>/dev/null

echo ""
echo "=== Test Complete ==="
echo "Review lwt_test.log for full message history"
echo ""
cat lwt_test.log
