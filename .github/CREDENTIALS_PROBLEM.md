# Critical Issue: Credentials and OTA Updates

## The Problem

**You identified a critical flaw in the current implementation.**

When credentials are compiled into firmware using `#define`:
```cpp
#define WIFI_SSID "MyWiFi"
#define WIFI_PASSWORD "MyPassword"
```

These become **part of the binary**. When you do an OTA update:

```
┌─────────────────────────────────┐
│ Step 1: Initial Upload (USB)   │
│ Firmware with YOUR credentials  │
│ Device connects ✓               │
└─────────────────────────────────┘
         ↓
┌─────────────────────────────────┐
│ Step 2: OTA Update              │
│ New firmware with PLACEHOLDERS  │
│ YOUR credentials OVERWRITTEN    │
└─────────────────────────────────┘
         ↓
┌─────────────────────────────────┐
│ Result: Device can't connect ❌ │
│ WiFi: "your-wifi-ssid"          │
│ Password: "your-wifi-password"  │
└─────────────────────────────────┘
```

**The device will be bricked and require USB access to fix.**

## Solutions

### Solution 1: Use GitHub Secrets (Quick Fix)

**Best for:** Single installation or all devices using same credentials

#### Setup:
1. Add secrets to GitHub repository:
   - Go to: Settings → Secrets and variables → Actions
   - Add: `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_SERVER`, etc.

2. Edit `.github/workflows/release.yml`:
   - Comment out "Option A" (placeholder credentials)
   - Uncomment "Option B" (GitHub Secrets)

3. Push a new release tag:
   ```bash
   git tag v1.0.1
   git push origin v1.0.1
   ```

**Result:** All released binaries contain YOUR credentials. OTA updates work perfectly.

**Pros:**
- ✅ No code changes needed
- ✅ Works immediately
- ✅ OTA updates preserve credentials

**Cons:**
- ❌ All devices must use same credentials
- ❌ Not suitable for public repositories
- ❌ Not suitable for multiple installations with different WiFi networks

---

### Solution 2: Use NVS (Preferences) - Proper Solution

**Best for:** Multiple devices, different credentials per device, production use

Store credentials in ESP32's Non-Volatile Storage (NVS) instead of compiling them in. Credentials persist across OTA updates because they're in a separate memory partition.

#### Implementation:

Create a new file `include/config_manager.h`:
```cpp
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Preferences.h>

class ConfigManager {
private:
    Preferences preferences;
    
public:
    // WiFi settings
    String wifiSSID;
    String wifiPassword;
    
    // MQTT settings
    String mqttServer;
    int mqttPort;
    String mqttUser;
    String mqttPassword;
    String mqttClientID;
    
    void begin() {
        preferences.begin("battery-mon", false);
        
        // Load WiFi credentials
        wifiSSID = preferences.getString("wifi_ssid", WIFI_SSID);
        wifiPassword = preferences.getString("wifi_pass", WIFI_PASSWORD);
        
        // Load MQTT credentials
        mqttServer = preferences.getString("mqtt_srv", MQTT_SERVER);
        mqttPort = preferences.getInt("mqtt_port", MQTT_PORT);
        mqttUser = preferences.getString("mqtt_user", MQTT_USER);
        mqttPassword = preferences.getString("mqtt_pass", MQTT_PASSWORD);
        mqttClientID = preferences.getString("mqtt_id", MQTT_CLIENT_ID);
        
        // On first run, save defaults to NVS
        if (!preferences.isKey("initialized")) {
            saveConfig();
            preferences.putBool("initialized", true);
        }
    }
    
    void saveConfig() {
        preferences.putString("wifi_ssid", wifiSSID);
        preferences.putString("wifi_pass", wifiPassword);
        preferences.putString("mqtt_srv", mqttServer);
        preferences.putInt("mqtt_port", mqttPort);
        preferences.putString("mqtt_user", mqttUser);
        preferences.putString("mqtt_pass", mqttPassword);
        preferences.putString("mqtt_id", mqttClientID);
    }
    
    void end() {
        preferences.end();
    }
};

#endif
```

Modify `main.cpp` to use ConfigManager:
```cpp
#include "config_manager.h"

ConfigManager config;

void setup() {
    Serial.begin(115200);
    
    // Load configuration from NVS
    config.begin();
    
    Serial.println("Configuration loaded:");
    Serial.println("WiFi SSID: " + config.wifiSSID);
    Serial.println("MQTT Server: " + config.mqttServer);
    
    // ... rest of setup
}

bool connectWiFi() {
    WiFi.begin(config.wifiSSID.c_str(), config.wifiPassword.c_str());
    // ... rest of function
}

bool connectMQTT() {
    mqttClient.setServer(config.mqttServer.c_str(), config.mqttPort);
    mqttClient.connect(config.mqttClientID.c_str(), 
                       config.mqttUser.c_str(), 
                       config.mqttPassword.c_str());
    // ... rest of function
}
```

**How it works:**
1. First upload: Reads compiled-in defaults, saves to NVS
2. Future boots: Reads from NVS (ignores compiled-in values)
3. OTA updates: New firmware reads from NVS, credentials preserved ✓

**Pros:**
- ✅ Credentials survive OTA updates
- ✅ Each device can have different credentials
- ✅ Can be updated via serial commands or web interface
- ✅ Industry standard approach

**Cons:**
- Requires code changes
- Slightly more complex

---

### Solution 3: WiFiManager (Most User-Friendly)

**Best for:** Non-technical users, field deployment

Use WiFiManager library for web-based configuration:

```cpp
#include <WiFiManager.h>

void setup() {
    WiFiManager wm;
    
    // Auto-connect or show config portal
    if (!wm.autoConnect("BatteryMonitor-Setup")) {
        Serial.println("Failed to connect");
        ESP.restart();
    }
    
    // Device is now connected with credentials saved in NVS
}
```

**How it works:**
1. First boot: Creates WiFi access point "BatteryMonitor-Setup"
2. User connects and enters WiFi/MQTT credentials via web interface
3. Credentials saved to NVS
4. Future boots and OTA updates use saved credentials

**Pros:**
- ✅ No credential files needed
- ✅ User-friendly web configuration
- ✅ Credentials survive OTA updates
- ✅ Can reset via button press

---

## Comparison

| Solution | OTA Safe | Multiple Devices | Public Repo | Ease of Use |
|----------|----------|------------------|-------------|-------------|
| GitHub Secrets | ✅ | Same creds only | ❌ | Easy |
| NVS (Preferences) | ✅ | ✅ | ✅ | Medium |
| WiFiManager | ✅ | ✅ | ✅ | Easy |

## Recommended Approach

### For Your Current Setup:
**Use Solution 1 (GitHub Secrets)** if:
- All your devices use the same WiFi network
- You have a private repository
- You want a quick fix

### For Production/Multiple Sites:
**Use Solution 2 (NVS)** because:
- Each device can have different credentials
- Proper separation of code and configuration
- Industry standard practice
- Credentials truly persist across OTA updates

## Example: How NVS Solves the Problem

```
┌─────────────────────────────────┐
│ First Upload (USB)              │
│ Firmware reads compiled defaults│
│ Saves to NVS partition         │
└─────────────────────────────────┘
         ↓
┌─────────────────────────────────┐
│ NVS Partition (separate)        │
│ WiFi: "MyNetwork"               │
│ Pass: "MyPassword"              │
│ [Persists across updates]       │
└─────────────────────────────────┘
         ↓
┌─────────────────────────────────┐
│ OTA Update                      │
│ New firmware installed          │
│ NVS partition UNTOUCHED         │
└─────────────────────────────────┘
         ↓
┌─────────────────────────────────┐
│ Device reboots                  │
│ New firmware reads NVS          │
│ Connects with YOUR credentials ✓│
└─────────────────────────────────┘
```

## Action Required

**You must choose one of these solutions before using OTA updates**, otherwise your devices will lose connectivity after the first OTA update.

Would you like me to implement Solution 2 (NVS) for you? It's the most robust approach.
