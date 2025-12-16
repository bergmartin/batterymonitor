#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config_manager.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#ifndef OTA_BASE_URL
#define OTA_BASE_URL "https://github.com/USERNAME/REPO/releases/download/"
#endif

#ifndef OTA_VERSION_URL
#define OTA_VERSION_URL "https://raw.githubusercontent.com/USERNAME/REPO/main/version.txt"
#endif

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0"
#endif

class OTAManager {
private:
    ConfigManager& config;
    bool otaRequested;
    String otaFilename;
    Preferences preferences;
    
    bool performHTTPUpdate(const String& filename);
    void saveOTATrigger(const String& filename);
    void clearOTATrigger();
    String checkLatestVersion();  // Check server for latest version
    bool isNewerVersion(const String& latestVersion, const String& currentVersion);
    
public:
    OTAManager(ConfigManager& cfg);
    
    void setup();
    void requestUpdate(const String& filename);
    bool isUpdateRequested() const;
    bool checkPendingOTA();  // Check if OTA was triggered while asleep
    bool checkForUpdates();  // Automatically check if newer version is available
    void handleUpdate();
    void loop();
};

#endif // OTA_MANAGER_H
