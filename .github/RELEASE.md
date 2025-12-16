# GitHub Actions Release Workflow

This repository is configured to automatically build and release firmware binaries when you push a version tag.

## How It Works

1. **Push a tag** starting with `v` (e.g., `v1.0.0`)
2. **GitHub Actions** automatically:
   - Builds both Lead-Acid and LiFePO4 firmware variants
   - Creates firmware files with version numbers
   - Generates SHA256 checksums
   - Creates a GitHub release
   - Uploads all binaries to the release

## Creating a Release

### Step 1: Commit your changes
```bash
git add .
git commit -m "Your commit message"
git push
```

### Step 2: Create and push a version tag
```bash
# Create a new version tag
git tag v1.0.0

# Or with annotation (recommended)
git tag -a v1.0.0 -m "Release version 1.0.0"

# Push the tag to GitHub
git push origin v1.0.0
```

### Step 3: Wait for the build
- Go to your repository on GitHub
- Click on **Actions** tab
- Watch the workflow run (takes ~2-5 minutes)
- When complete, check the **Releases** section

## Files Generated

Each release includes:
- `batterymonitor-leadacid-v1.0.0.bin` - Lead-Acid variant (with version)
- `batterymonitor-leadacid.bin` - Lead-Acid variant (generic name)
- `batterymonitor-lifepo4-v1.0.0.bin` - LiFePO4 variant (with version)
- `batterymonitor-lifepo4.bin` - LiFePO4 variant (generic name)
- `checksums.txt` - SHA256 checksums for verification

## Using "latest" for OTA Updates

The workflow creates both versioned and generic filenames. This allows you to:

### Option 1: Use specific version URLs
```ini
; In platformio.ini
-D OTA_BASE_URL='"https://github.com/USER/REPO/releases/download/v1.0.0/"'
```

### Option 2: Use "latest" for automatic updates (Recommended)
```ini
; In platformio.ini
-D OTA_BASE_URL='"https://github.com/USER/REPO/releases/download/"'
```

Then trigger with:
```bash
mosquitto_pub -h BROKER -t "battery/monitor/ota" -m "batterymonitor-leadacid.bin"
```

## Credential Files

The workflow creates placeholder credential files for building. **Users must configure their own credentials** before uploading to their devices:

1. Copy example files:
   ```bash
   cp include/wifi_credentials.h.example include/wifi_credentials.h
   cp include/mqtt_credentials.h.example include/mqtt_credentials.h
   ```

2. Edit with your actual credentials

3. Build and upload locally:
   ```bash
   pio run -e esp32dev-leadacid -t upload
   ```

## Versioning Guidelines

Follow [Semantic Versioning](https://semver.org/):
- `v1.0.0` - Major.Minor.Patch
- `v1.0.1` - Bug fixes
- `v1.1.0` - New features (backward compatible)
- `v2.0.0` - Breaking changes

## Troubleshooting

### Workflow fails to build
- Check the Actions tab for error details
- Ensure `platformio.ini` is valid
- Verify all source files compile locally first

### Release not created
- Make sure tag starts with `v`
- Check repository has Actions enabled
- Verify you have push access to the repository

### Wrong battery type
Make sure to specify the correct firmware file:
- Lead-Acid batteries: `batterymonitor-leadacid.bin`
- LiFePO4 batteries: `batterymonitor-lifepo4.bin`

## Manual Release (if needed)

If you prefer not to use GitHub Actions:

```bash
# Build locally
pio run -e esp32dev-leadacid
pio run -e esp32dev-lifepo4

# Create release manually on GitHub
# Upload .pio/build/esp32dev-leadacid/firmware.bin
# Upload .pio/build/esp32dev-lifepo4/firmware.bin
```
