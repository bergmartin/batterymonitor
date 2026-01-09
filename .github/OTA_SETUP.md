# Automatic OTA Deployment Setup

This document explains how to configure GitHub Actions to automatically deploy firmware updates to your ESP32 devices via MQTT when you push a tag.

## Overview

The CI/CD workflow automatically:
1. **Nightly Builds**: Builds firmware every night at 2 AM UTC (stored as artifacts for 30 days)
2. **Tag Releases**: Creates GitHub releases when you push tags (only from `main` branch)
3. **OTA Deployment**: Publishes firmware URL to MQTT topic to trigger device updates

## GitHub Secrets Configuration

To enable automatic OTA deployment, configure these secrets in your repository:

**Settings → Secrets and variables → Actions → New repository secret**

### Required Secrets

| Secret Name | Description | Example |
|-------------|-------------|---------|
| `OTA_MQTT_SERVER` | MQTT broker hostname | `cloudy.mbergeron.ca` |
| `OTA_MQTT_PORT` | MQTT broker port (usually 8883 for TLS) | `8883` |
| `OTA_MQTT_USER` | MQTT username for publishing | `ota_updater` |
| `OTA_MQTT_PASSWORD` | MQTT password | `your-secure-password` |
| `OTA_MQTT_TOPIC` | Topic to publish OTA URLs (optional) | `battery/monitor/ota` |

**Note:** If secrets are not configured, the workflow will skip the OTA deployment step and only create the GitHub release.

## Workflow Triggers

### 1. Tag Pushes (Releases)
```bash
# Create and push a tag from main branch
git checkout main
git tag v0.3.4
git push origin v0.3.4
```

**What happens:**
- ✅ Validates tag is on `main` branch
- ✅ Builds firmware with version `0.3.4`
- ✅ Creates GitHub release with firmware.bin
- ✅ Publishes OTA URL to MQTT topic
- ✅ Devices subscribed to MQTT topic auto-update

### 2. Nightly Builds (Scheduled)
Automatically runs at 2 AM UTC daily.

**What happens:**
- ✅ Builds firmware with version `nightly-YYYYMMDD-githash`
- ✅ Uploads firmware as GitHub artifact (30 day retention)
- ❌ Does NOT create release
- ❌ Does NOT trigger OTA deployment

### 3. Manual Trigger
Use the "Run workflow" button in GitHub Actions tab.

**What happens:**
- Same as nightly build
- Useful for testing or on-demand builds

## Tag Naming Convention

- **Format:** `v<major>.<minor>.<patch>`
- **Examples:** `v1.0.0`, `v0.3.4`, `v2.1.5`
- **Main branch only:** All tags must be created on the `main` branch

## How OTA Deployment Works

1. **Build Complete:** Firmware uploaded to GitHub release
2. **MQTT Publish:** Workflow publishes firmware URL to MQTT topic:
   ```
   Topic: battery/monitor/ota
   Payload: https://github.com/USER/REPO/releases/download/v0.3.4/firmware.bin
   QoS: 1 (at least once)
   Retain: true
   ```
3. **Device Detection:** ESP32 devices subscribed to topic receive URL
4. **Auto Update:** Devices download and install firmware on next wake cycle

## Testing the Setup

### Test MQTT Connection
```bash
# Install mosquitto clients
sudo apt-get install mosquitto-clients

# Test publish (replace with your credentials)
mosquitto_pub \
  -h cloudy.mbergeron.ca \
  -p 8883 \
  -u ota_updater \
  -P "your-password" \
  --capath /etc/ssl/certs \
  -t "battery/monitor/ota" \
  -m "https://github.com/USER/REPO/releases/download/v0.3.4/firmware.bin" \
  -q 1 \
  -r
```

### Monitor Device Response
```bash
# Subscribe to device status
mosquitto_sub \
  -h cloudy.mbergeron.ca \
  -p 8883 \
  -u esp32_user \
  -P "your-password" \
  --capath /etc/ssl/certs \
  -t "esp32-battery-monitor-0_availability/state" \
  -v
```

## Security Best Practices

1. **Separate MQTT User:** Use a dedicated `ota_updater` user with publish-only permissions
2. **ACL Configuration:** Restrict `ota_updater` to only publish to OTA topic
3. **Strong Passwords:** Use generated passwords (20+ characters)
4. **TLS/SSL:** Always use port 8883 with TLS encryption
5. **Secret Rotation:** Rotate MQTT credentials periodically

### Example Mosquitto ACL
```
# /etc/mosquitto/conf.d/acl.conf

# OTA updater - publish only
user ota_updater
topic write battery/monitor/ota

# ESP32 devices - read OTA updates
user esp32_user
topic read battery/monitor/ota
topic readwrite esp32-battery-monitor-+/#
```

## Troubleshooting

### Workflow Fails: "Tags can only be created on main branch"
**Solution:** 
```bash
git checkout main
git merge your-branch
git tag v0.3.4
git push origin main
git push origin v0.3.4
```

### OTA Deployment Skipped
**Check:** Are GitHub secrets configured?
```bash
# View workflow run to see if secrets are set
# Look for: "⚠️ OTA_MQTT_SERVER secret not set"
```

### Devices Not Updating
1. **Check MQTT topic:** Verify devices subscribe to correct topic
2. **Check retain flag:** Ensure message is retained for sleeping devices
3. **Check device logs:** Monitor serial output during wake cycle
4. **Test MQTT manually:** Use `mosquitto_pub` to test

### Build Fails
1. **Check credentials:** Ensure `wifi_credentials.h` and `mqtt_credentials.h` are created
2. **Clear cache:** Delete `.pio` folder and rebuild
3. **Check PlatformIO version:** Update to latest

## Workflow File Location

`.github/workflows/release.yml`

## Related Documentation

- [OTA.md](../doc/OTA.md) - Device-side OTA implementation
- [MQTT.md](../doc/MQTT.md) - MQTT integration details
- [AGENTS.md](../AGENTS.md) - Development best practices
