# Credential Management for GitHub Actions

## Understanding How Credentials Work

### GitHub-Built Binaries
The firmware binaries built by GitHub Actions contain **placeholder credentials** by default. This is intentional and correct for most use cases.

### Why Placeholder Credentials Are OK

**OTA updates don't need real credentials because:**
1. OTA only updates the firmware code
2. Credentials are stored separately in ESP32 flash memory
3. Your existing WiFi/MQTT credentials are preserved during OTA

**Example workflow:**
```
Initial Setup (USB) → Device has your credentials
     ↓
OTA Update (MQTT) → Firmware updates, credentials stay
     ↓
Device reboots → Uses existing credentials + new firmware
```

## Setup Options

### Option 1: Placeholder Credentials (Recommended)

**Best for:** Multiple users, open-source projects, when devices have different credentials

**How it works:**
1. GitHub builds firmware with placeholder values
2. Each user configures credentials locally
3. First upload via USB with real credentials
4. Future OTA updates preserve those credentials

**Steps:**
```bash
# 1. Clone repository
git clone https://github.com/YOUR_USERNAME/batterymonitor.git
cd batterymonitor

# 2. Configure YOUR credentials locally
cp include/wifi_credentials.h.example include/wifi_credentials.h
cp include/mqtt_credentials.h.example include/mqtt_credentials.h

# Edit files with your actual credentials
nano include/wifi_credentials.h
nano include/mqtt_credentials.h

# 3. Build and upload (USB) - includes YOUR credentials
pio run -e esp32dev-leadacid -t upload

# 4. Future updates via OTA - credentials preserved
mosquitto_pub -h BROKER -t "battery/monitor/ota" -m "batterymonitor-leadacid.bin"
```

**GitHub release binaries contain:** Placeholder credentials  
**Your device contains:** Your real credentials (from step 3)  
**OTA updates:** Don't touch credentials

### Option 2: GitHub Secrets (For Single Deployment)

**Best for:** Single installation, all devices use same credentials, private repository

**How it works:**
1. Store credentials as GitHub Secrets
2. GitHub builds firmware with your actual credentials
3. Download and upload directly - no local configuration needed

**Setup:**

#### A. Add Secrets to GitHub
1. Go to repository **Settings** → **Secrets and variables** → **Actions**
2. Click **New repository secret**
3. Add each secret:
   - `WIFI_SSID` = `your-wifi-name`
   - `WIFI_PASSWORD` = `your-wifi-password`
   - `MQTT_SERVER` = `mqtt.example.com`
   - `MQTT_PORT` = `1883`
   - `MQTT_USER` = `mqtt-user`
   - `MQTT_PASSWORD` = `mqtt-password`
   - `MQTT_CLIENT_ID` = `battery-monitor`

#### B. Update Workflow File
Edit `.github/workflows/release.yml` and uncomment the "Option B" section:

```yaml
# Uncomment this section:
cat > include/wifi_credentials.h << EOF
#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

#define WIFI_SSID "${{ secrets.WIFI_SSID }}"
#define WIFI_PASSWORD "${{ secrets.WIFI_PASSWORD }}"

#endif
EOF
```

(Comment out Option A - the placeholder section)

#### C. Use the Release
```bash
# 1. Download firmware from GitHub release
wget https://github.com/USER/REPO/releases/latest/download/batterymonitor-leadacid.bin

# 2. Upload directly - no credential configuration needed!
pio run -e esp32dev-leadacid -t upload

# 3. OTA updates work immediately
mosquitto_pub -h BROKER -t "battery/monitor/ota" -m "batterymonitor-leadacid.bin"
```

**⚠️ Security Warning:**
- Anyone with access to your release binaries can extract credentials
- Only use for private repositories
- Don't share release binaries publicly

## Which Option Should You Use?

| Scenario | Best Option | Why |
|----------|-------------|-----|
| Open source project | Option 1 (Placeholder) | Users configure their own credentials |
| Multiple locations | Option 1 (Placeholder) | Each site has different WiFi/MQTT |
| Personal project | Either | Your choice |
| Same credentials everywhere | Option 2 (Secrets) | Convenient, pre-configured |
| Public repository | Option 1 (Placeholder) | Never expose real credentials |

## FAQ

### Q: Why don't GitHub binaries connect to WiFi?
**A:** They contain placeholder credentials. This is intentional - configure locally before first upload.

### Q: Will OTA updates lose my credentials?
**A:** No! OTA only updates code, not credentials. Your WiFi/MQTT settings are preserved.

### Q: Can I change credentials via OTA?
**A:** No. Credentials must be changed locally and re-uploaded via USB.

### Q: What if I have multiple devices?
**A:** With Option 1, each device can have different credentials configured during initial USB upload.

### Q: Are my credentials secure?
**A:** 
- **Option 1:** Credentials never leave your computer (most secure)
- **Option 2:** Credentials stored in GitHub Secrets (encrypted at rest)
- Both options: Credentials compiled into firmware (not easily readable)

## Troubleshooting

### Device won't connect after OTA update
```
Problem: Using GitHub binary on fresh device
Solution: Configure credentials locally first, upload via USB
```

### Want to change WiFi network
```
Problem: Need to update credentials
Solution: 
1. Edit wifi_credentials.h locally
2. Upload via USB (not OTA)
3. Device now uses new credentials
```

### Multiple devices, different networks
```
Problem: Each location has different WiFi
Solution: Use Option 1
- Configure credentials locally for each device
- Upload each device via USB with its specific credentials
- All devices can use same OTA firmware updates
```

## Example: Complete Setup

```bash
# Day 1: Initial setup (USB required)
git clone https://github.com/myuser/batterymonitor.git
cd batterymonitor
cp include/*.h.example include/
nano include/wifi_credentials.h  # Add your WiFi
nano include/mqtt_credentials.h  # Add your MQTT
pio run -e esp32dev-leadacid -t upload  # Upload via USB

# Day 30: Firmware update available (no USB needed!)
# Device wakes up, checks MQTT, receives update command
mosquitto_pub -h mqtt.home.local \
  -t "battery/monitor/ota" \
  -m "batterymonitor-leadacid.bin"

# Device downloads from GitHub, installs, reboots
# Credentials unchanged, firmware updated ✓
```
