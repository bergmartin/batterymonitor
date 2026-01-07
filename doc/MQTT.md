# MQTT Integration Guide

This document explains how to configure and use the MQTT functionality for publishing battery monitoring data.

## Overview

The battery monitor publishes readings to an MQTT broker every 1 hour (configurable). This enables:
- Remote monitoring via MQTT clients (Home Assistant, Node-RED, etc.)
- Historical data logging
- Alerting and automation based on battery status
- Integration with smart home systems

## MQTT Topics

All messages are published to topics under the base: `battery/monitor/`

### Individual Topics

- `battery/monitor/voltage` - Battery voltage (e.g., "12.45")
- `battery/monitor/percentage` - Battery percentage (e.g., "85.2")
- `battery/monitor/status` - Battery status ("FULL", "GOOD", "LOW", "CRITICAL")
- `battery/monitor/type` - Battery type ("Lead-Acid" or "LiFePO4")
- `battery/monitor/boot_count` - Number of times ESP32 has woken up

### JSON Topic

- `battery/monitor/json` - All data in JSON format:
  ```json
  {
    "voltage": 12.45,
    "percentage": 85.2,
    "status": "GOOD",
    "type": "Lead-Acid",
    "boot": 42,
    "rssi": -65
  }
  ```

### Configuration Topics

- `battery/monitor/config/battery_type` (QoS 1)
  - Payload: `leadacid` or `lifepo4` (case-insensitive)
  - Effect: Updates battery chemistry thresholds and persists to NVS.
  - Acknowledgement: Device publishes current type to `{hostname}_battery_type/state`.

## Configuration

### 1. Set Up Credentials

Create credential files from the templates:

```bash
cd include/
cp wifi_credentials.h.example wifi_credentials.h
cp mqtt_credentials.h.example mqtt_credentials.h
```

### 2. Configure WiFi

Edit `include/wifi_credentials.h`:

```cpp
#define WIFI_SSID "YourWiFiNetwork"
#define WIFI_PASSWORD "YourWiFiPassword"
```

### 3. Configure MQTT Broker

Edit `include/mqtt_credentials.h`:

```cpp
#define MQTT_SERVER "mqtt.example.com"  // Broker hostname or IP
#define MQTT_PORT 1883                   // Standard MQTT port
#define MQTT_USER "username"             // Leave "" if no auth
#define MQTT_PASSWORD "password"         // Leave "" if no auth
#define MQTT_CLIENT_ID "esp32-battery-monitor"
```

### 4. Build and Upload

```bash
pio run -e esp32dev --target upload
# or for LiFePO4
pio run -e esp32dev -D BATTERY_TYPE=BATTERY_TYPE_LIFEPO4 --target upload
```

## MQTT Broker Options

### Self-Hosted (Recommended for local use)

**Mosquitto** - Lightweight, open-source MQTT broker

Installation:
```bash
# Ubuntu/Debian
sudo apt install mosquitto mosquitto-clients

# macOS
brew install mosquitto
```

Start broker:
```bash
mosquitto -v
```

Configuration in `mqtt_credentials.h`:
```cpp
#define MQTT_SERVER "192.168.1.100"  // Your server IP
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_CLIENT_ID "esp32-battery-monitor"
```

### Cloud MQTT Services

1. **CloudMQTT** (https://www.cloudmqtt.com/)
   - Free tier available
   - Easy setup
   - Global CDN

2. **HiveMQ Cloud** (https://www.hivemq.com/mqtt-cloud-broker/)
   - Free tier: 100 connections
   - Good documentation
   - TLS support

3. **AWS IoT Core** (https://aws.amazon.com/iot-core/)
   - Scalable
   - Integrates with AWS services
   - Requires certificates

### Public Test Broker (Testing only)

**test.mosquitto.org** - Public MQTT broker for testing

⚠️ **WARNING**: No security, data is public!

```cpp
#define MQTT_SERVER "test.mosquitto.org"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_CLIENT_ID "esp32-battery-monitor-test"
```

## Testing MQTT

### Subscribe to Topics (Linux/macOS)

Install mosquitto-clients:
```bash
# Ubuntu/Debian
sudo apt install mosquitto-clients

# macOS
brew install mosquitto
```

Subscribe to all battery topics:
```bash
mosquitto_sub -h mqtt.example.com -t "battery/monitor/#" -v
```

Subscribe to JSON topic only:
```bash
mosquitto_sub -h mqtt.example.com -t "battery/monitor/json"
```

With authentication:
```bash
mosquitto_sub -h mqtt.example.com -u username -P password -t "battery/monitor/#"
```

### MQTT Client Apps

1. **MQTT Explorer** (https://mqtt-explorer.com/)
   - GUI application for Windows/macOS/Linux
   - Visual topic hierarchy
   - Real-time updates

2. **MQTT.fx** (https://mqttfx.jensd.de/)
   - Desktop client
   - Good for testing

3. **Home Assistant**
   - Automatic MQTT discovery
   - Built-in dashboards
   - Automation support

## Home Assistant Integration

### Auto-Discovery Configuration

Add to Home Assistant `configuration.yaml`:

```yaml
mqtt:
  sensor:
    - name: "Battery Voltage"
      state_topic: "battery/monitor/voltage"
      unit_of_measurement: "V"
      device_class: voltage
      state_class: measurement
      
    - name: "Battery Percentage"
      state_topic: "battery/monitor/percentage"
      unit_of_measurement: "%"
      device_class: battery
      state_class: measurement
      
    - name: "Battery Status"
      state_topic: "battery/monitor/status"
      icon: mdi:battery
```

### Automation Example

```yaml
automation:
  - alias: "Battery Low Alert"
    trigger:
      - platform: mqtt
        topic: "battery/monitor/status"
        payload: "LOW"
    action:
      - service: notify.mobile_app
        data:
          message: "Battery is low! Voltage: {{ states('sensor.battery_voltage') }}V"
```

## Power Consumption

With MQTT enabled:
- **Awake time**: ~10-15 seconds (WiFi + MQTT connection + publish)
- **Active current**: ~80-160 mA
- **Deep sleep**: ~0.01 mA
- **Average consumption**: ~0.3-0.5 mA (with 1-hour intervals)

WiFi connection adds ~5-8 seconds to wake time.

## Troubleshooting

### WiFi Won't Connect

1. Check credentials in `wifi_credentials.h`
2. Verify WiFi signal strength at ESP32 location
3. Check 2.4GHz network (ESP32 doesn't support 5GHz)
4. Increase `WIFI_TIMEOUT_MS` in `battery_config.h`

Serial output:
```
Connecting to WiFi...... Failed!
```

### MQTT Won't Connect

1. Verify broker is running: `mosquitto -v`
2. Check broker IP/hostname in `mqtt_credentials.h`
3. Test broker with mosquitto_pub: `mosquitto_pub -h mqtt.example.com -t test -m "hello"`
4. Verify firewall allows port 1883
5. Check MQTT credentials (username/password)

Serial output:
```
Connecting to MQTT broker..... Failed!
```

### No Data Published

1. Check serial monitor for error messages
2. Verify topics with `mosquitto_sub -h mqtt.example.com -t "#" -v`
3. Check retained message flag
4. Increase `MQTT_TIMEOUT_MS` in `battery_config.h`

### Memory Issues

If ESP32 runs out of memory with MQTT enabled:
1. Reduce `SAMPLE_COUNT` in `battery_config.h`
2. Disable serial debug output
3. Use shorter MQTT_CLIENT_ID

## Customization

### Change Base Topic

Edit in `lib/BatteryMonitor/battery_config.h`:

```cpp
constexpr char MQTT_TOPIC_BASE[] = "home/garage/battery";
```

Results in topics like:
- `home/garage/battery/voltage`
- `home/garage/battery/json`

### Adjust Timeouts

Edit in `lib/BatteryMonitor/battery_config.h`:

```cpp
constexpr unsigned long WIFI_TIMEOUT_MS = 10000;  // 10 seconds (default)
constexpr unsigned long MQTT_TIMEOUT_MS = 10000;  // 10 seconds
```

### QoS and Retained Messages

Modify in `src/main.cpp` (publishToMQTT function):

```cpp
// QoS 0, not retained
mqttClient.publish(topic, value, false);

// QoS 1, retained
mqttClient.publish(topic, value, 1, true);
```

## Security Considerations

1. **Never commit credentials**: The `.gitignore` protects credential files
2. **Use authentication**: Set MQTT_USER and MQTT_PASSWORD
3. **Use TLS**: Change MQTT_PORT to 8883 and add TLS support
4. **Restrict topics**: Configure broker ACLs
5. **Local network**: Keep MQTT broker on local network if possible

## Performance Tips

1. **Connection time**: WiFi connection is the slowest part (~5-8 seconds)
2. **Retained messages**: Use retained flag so clients get last value immediately
3. **JSON vs individual**: JSON is more efficient for multiple subscribers
4. **QoS 0**: Fastest, no acknowledgment (use for non-critical data)
5. **Keep awake time short**: Disconnect immediately after publishing

## Additional Resources

- MQTT Protocol: https://mqtt.org/
- Mosquitto Documentation: https://mosquitto.org/documentation/
- Home Assistant MQTT: https://www.home-assistant.io/integrations/mqtt/
- PubSubClient Library: https://pubsubclient.knolleary.net/
