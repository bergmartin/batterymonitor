#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "config_manager.h"

// Forward declaration
class DisplayManager;

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0"
#endif

class OTAManager {
private:
    ConfigManager& config;
    DisplayManager* display;
    bool otaRequested;
    String otaFilename;
    Preferences preferences;
    
    bool performHTTPUpdate(const String& filename);
    void saveOTATrigger(const String& filename);
    bool isNewerVersion(const String& latestVersion, const String& currentVersion);
    
public:
    OTAManager(ConfigManager& cfg, DisplayManager* disp = nullptr);
    
    void setup();
    void requestUpdate(const String& filename);
    void clearOTATrigger();  // Public method to clear pending OTA trigger
    bool isUpdateRequested() const;
    bool checkPendingOTA();  // Check if OTA was triggered while asleep
    bool checkForUpdates();  // Automatically check if newer version is available
    void handleUpdate();
    void loop();
};

#endif // OTA_MANAGER_H
