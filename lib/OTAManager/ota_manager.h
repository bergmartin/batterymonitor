#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "config_manager.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#ifndef OTA_BASE_URL
#define OTA_BASE_URL "https://github.com/USERNAME/REPO/releases/download/"
#endif

class OTAManager {
private:
    ConfigManager& config;
    bool otaRequested;
    String otaFilename;
    
    bool performHTTPUpdate(const String& filename);
    
public:
    OTAManager(ConfigManager& cfg);
    
    void setup();
    void requestUpdate(const String& filename);
    bool isUpdateRequested() const;
    void handleUpdate();
    void loop();
};

#endif // OTA_MANAGER_H
