# MQTT Device Health Monitor

An MQTT-based device monitoring system written in C that collects and publishes system metrics (memory, network, uptime) to an MQTT broker.

## What It Does

The device agent collects real system metrics from your Linux machine and publishes them via MQTT:
- **Memory usage** (total, used, free)
- **Network statistics** (bytes sent/received)
- **System uptime**
- **Device status** (online/offline)
- **Heartbeat messages** (every 30 seconds)

It uses MQTT's **Last Will and Testament** (LWT) feature to automatically detect when a device crashes or loses connection.

## Prerequisites

```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients libmosquitto-dev build-essential
```

## Build

```bash
make        # Build everything
make clean  # Clean build files
```

## Run

### Start the device agent:
```bash
./bin/device_agent <device-name>
```

Example:
```bash
./bin/device_agent my-laptop
```

Press `Ctrl+C` to stop gracefully.

### Monitor all device messages:
```bash
mosquitto_sub -h localhost -t 'devices/#' -v
```

### Monitor only telemetry (system metrics):
```bash
mosquitto_sub -h localhost -t 'devices/+/telemetry'
```

### Monitor only status messages:
```bash
mosquitto_sub -h localhost -t 'devices/+/status' -v
```

## Example Output

**Telemetry (every 10 seconds):**
```json
{
  "timestamp": 1770569487,
  "device_id": "my-laptop",
  "uptime": 43006,
  "cpu_temp": 0.00,
  "memory": {
    "total": 8171737088,
    "used": 4005769216,
    "free": 4165967872
  },
  "network": {
    "rx_bytes": 1385641586,
    "tx_bytes": 955780941
  }
}
```

**Status messages:**
```json
{"state": "online", "timestamp": 1770505333}
{"state": "offline", "timestamp": 1770505343}
```

## Testing LWT (Last Will & Testament)

To test automatic failure detection:

```bash
# Terminal 1: Subscribe to status
mosquitto_sub -h localhost -t 'devices/+/status' -v

# Terminal 2: Start device
./bin/device_agent test-device &

# Terminal 3: Kill it abruptly (simulates crash)
pkill -9 device_agent
```

You'll see the broker automatically publish an offline message when the device dies.

