# OTA (Over-The-Air) Updates

The battery monitor supports two methods for OTA updates:

## 1. HTTP OTA from GitHub (Recommended)

Download and install firmware directly from a GitHub release. The base URL is compiled into the firmware for security - only the filename is sent via MQTT.

### Setup

1. Configure the base URL in `platformio.ini`:
```ini
build_flags = 
  -D BATTERY_TYPE=LEAD_ACID
  -D OTA_BASE_URL='"https://github.com/USERNAME/REPO/releases/download/latest/"'
```

2. Build your firmware and create a GitHub release with the `.bin` file

### Usage

Publish **only the filename** to the MQTT topic:

```bash
# Send just the filename - base URL is compiled into firmware
mosquitto_pub -h YOUR_MQTT_BROKER \
  -t "battery/monitor/ota" \
  -m "firmware.bin"

# Or with version-specific filename
mosquitto_pub -h YOUR_MQTT_BROKER \
  -t "battery/monitor/ota" \
  -m "batterymonitor-v1.0.1.bin"
```

**How it works:**
1. Device receives MQTT message with **filename only**
2. Constructs full URL: `OTA_BASE_URL + filename`
3. Downloads firmware binary from the constructed URL
4. Installs update and reboots automatically
5. Update completes in ~30-60 seconds

**Security Features:**
- ✅ Base URL is compiled into firmware (cannot be changed remotely)
- ✅ Only accepts filenames - rejects paths, URLs, or special characters
- ✅ Prevents unauthorized firmware sources
- ✅ No risk of arbitrary URL injection

**Advantages:**
- Secure - base URL locked at compile time
- No direct network access needed to ESP32
- Can update devices anywhere (through MQTT broker)
- Automated deployment possible via GitHub Actions
- Works great for remote/field deployments

## 2. ArduinoOTA (Network Upload)

Direct network upload using PlatformIO or Arduino IDE.

### Usage

Trigger OTA mode without a URL:

```bash
# Trigger OTA mode (no URL = ArduinoOTA mode)
mosquitto_pub -h YOUR_MQTT_BROKER \
  -t "battery/monitor/ota" \
  -m "update"
```

Then upload via PlatformIO:

```bash
# Configure platformio.ini first:
# upload_protocol = espota
# upload_port = 192.168.1.XXX

pio run -t upload
```

**How it works:**
1. Device enters OTA mode and waits 5 minutes
2. Upload firmware via WiFi using PlatformIO
3. Device installs and reboots

**Advantages:**
- Good for local development
- No web server needed
- Protected by optional password

## Getting the Firmware Binary

### From PlatformIO Build

```bash
# Build the project
pio run

# Binary location:
.pio/build/esp32dev/firmware.bin
```

### Host on GitHub Releases

1. Create a new release on GitHub
2. Attach the `firmware.bin` file
3. Configure base URL in `platformio.ini`:
   ```ini
   -D OTA_BASE_URL='"https://github.com/USERNAME/REPO/releases/download/v1.0.1/"'
   ```
4. Send just the filename via MQTT:
   ```bash
   mosquitto_pub -h BROKER -t "battery/monitor/ota" -m "firmware.bin"
   ```

### Using "latest" Tag

For automatic latest release:
```ini
-D OTA_BASE_URL='"https://github.com/USERNAME/REPO/releases/latest/download/"'
```

Then always use the same filename in your releases:
```bash
mosquitto_pub -h BROKER -t "battery/monitor/ota" -m "batterymonitor.bin"
```

### Temporary Local Server (Testing)

For local testing, configure local base URL:
```ini
-D OTA_BASE_URL='"http://192.168.1.100:8000/"'
```

Then serve the firmware:
```bash
# Python 3
cd .pio/build/esp32dev/
python -m http.server 8000
```

Trigger update:
```bash
mosquitto_pub -h BROKER -t "battery/monitor/ota" -m "firmware.bin"
```

## Security Considerations

### Compile-Time Base URL
- Base URL is **hardcoded at compile time** and cannot be changed remotely
- Only filenames are accepted via MQTT (no paths, URLs, or special characters)
- Prevents injection of malicious firmware sources
- Must rebuild firmware to change base URL

### Input Validation
The device automatically rejects:
- Full URLs (containing `http://` or `https://`)
- Paths (containing `/` or `\`)
- Special characters (containing `:`)
- This prevents arbitrary firmware downloads

### HTTPS Recommendation
- Use HTTPS URLs for production (GitHub releases use HTTPS)
- HTTP acceptable for local testing only
- GitHub automatically provides secure TLS connections

### Password Protection

Uncomment in `main.cpp`:
```cpp
ArduinoOTA.setPassword("your_password");
```

## Automation with GitHub Actions

Example workflow with versioned releases:

Create `.github/workflows/release.yml`:

```yaml
name: Build and Release Firmware

on:
  push:
    tags:
      - 'v*'

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.x'
      
      - name: Install PlatformIO
        run: pip install platformio
      
      - name: Build firmware
        run: |
          # Update OTA_BASE_URL to point to this release
          TAG=${GITHUB_REF#refs/tags/}
          sed -i "s|releases/download/latest/|releases/download/${TAG}/|g" platformio.ini
          pio run -e esp32dev
      
      - name: Rename firmware with version
        run: |
          TAG=${GITHUB_REF#refs/tags/}
          cp .pio/build/esp32dev/firmware.bin batterymonitor-${TAG}.bin
      
      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            batterymonitor-${{ github.ref_name }}.bin
            .pio/build/esp32dev/firmware.bin
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### Using "latest" for auto-updates

For automatic latest version (recommended):

1. Configure base URL to always use `latest`:
```ini
-D OTA_BASE_URL='"https://github.com/USERNAME/REPO/releases/latest/download/"'
```

2. Always name your firmware file the same (e.g., `batterymonitor.bin`)

3. Create releases - GitHub automatically updates "latest"

4. Trigger updates with just the filename:
```bash
mosquitto_pub -h BROKER -t "battery/monitor/ota" -m "batterymonitor.bin"
```

All devices automatically get the newest firmware from the latest release!

## Troubleshooting

### Update Fails
- Check base URL is correct in `platformio.ini`
- Verify filename matches exactly (case-sensitive)
- Ensure firmware file exists at `BASE_URL + filename`
- Check WiFi connection is stable
- Verify firmware is built for correct board
- Check serial output for detailed error messages

### Invalid Filename Error
If you see "ERROR: Invalid filename", the message contained:
- A path separator (`/` or `\`)
- A colon (`:`)
- A full URL
- Only send the filename: `firmware.bin` not `https://...`

### Device Doesn't Respond
- Verify MQTT topic matches: `battery/monitor/ota`
- Check device is awake (not in deep sleep)
- Wait for next wake cycle (default: 4 hours)
- Verify MQTT message was received (check broker logs)

### Progress Monitoring
- Watch serial output during update
- Check MQTT for status messages
- HTTP updates show download progress

## Power Considerations

During OTA updates, the device:
- Stays awake (80-160 mA current draw)
- ArduinoOTA mode: 5 minutes maximum
- HTTP update: ~30-60 seconds typical
- Returns to deep sleep after completion
