#include "ota_manager.h"
#include "battery_config.h"

OTAManager::OTAManager(ConfigManager& cfg) 
    : config(cfg), otaRequested(false), otaFilename("") {}

void OTAManager::saveOTATrigger(const String& filename) {
    preferences.begin("ota", false);
    preferences.putBool("pending", true);
    preferences.putString("filename", filename);
    preferences.end();
    Serial.println("OTA trigger saved to persistent storage");
}

void OTAManager::clearOTATrigger() {
    preferences.begin("ota", false);
    preferences.putBool("pending", false);
    preferences.putString("filename", "");
    preferences.end();
    Serial.println("OTA trigger cleared from persistent storage");
}

bool OTAManager::checkPendingOTA() {
    preferences.begin("ota", true);  // Read-only
    bool pending = preferences.getBool("pending", false);
    String filename = preferences.getString("filename", "");
    preferences.end();
    
    if (pending) {
        Serial.println("╔═══════════════════════════════╗");
        Serial.println("║  Pending OTA Update Detected  ║");
        Serial.println("╚═══════════════════════════════╝");
        Serial.print("Filename: ");
        Serial.println(filename.length() > 0 ? filename : "(ArduinoOTA mode)");
        
        otaRequested = true;
        otaFilename = filename;
        return true;
    }
    return false;
}

void OTAManager::setup() {
    // Configure OTA settings
    ArduinoOTA.setHostname(config.mqttClientID.c_str());
    
    // Optionally set password for OTA updates
    // ArduinoOTA.setPassword("admin");
    
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {  // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("\n╔═══════════════════════════════╗");
        Serial.println("║   OTA Update Started          ║");
        Serial.println("╚═══════════════════════════════╝");
        Serial.println("Type: " + type);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n╔═══════════════════════════════╗");
        Serial.println("║   OTA Update Complete         ║");
        Serial.println("╚═══════════════════════════════╝");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("\nError[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });
    
    ArduinoOTA.begin();
    Serial.println("OTA service initialized");
    Serial.print("Hostname: ");
    Serial.println(config.mqttClientID);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void OTAManager::requestUpdate(const String& filename) {
    otaRequested = true;
    otaFilename = filename;
    saveOTATrigger(filename);  // Persist for next boot
    Serial.print("OTA requested with filename: ");
    Serial.println(filename.length() > 0 ? filename : "(ArduinoOTA mode)");
}

bool OTAManager::isUpdateRequested() const {
    return otaRequested;
}

void OTAManager::handleUpdate() {
    if (!otaRequested) return;
    
    Serial.println("\n╔═══════════════════════════════╗");
    Serial.println("║   Entering OTA Mode           ║");
    Serial.println("╚═══════════════════════════════╝");
    
    // Check if filename is provided for HTTP update
    if (otaFilename.length() > 0) {
        Serial.println("Mode: HTTP Update from GitHub");
        
        // Clear the persistent trigger BEFORE attempting update
        // This prevents re-triggering if update succeeds and device reboots
        clearOTATrigger();
        Serial.println("Cleared OTA trigger before update attempt");
        
        if (performHTTPUpdate(otaFilename)) {
            Serial.println("HTTP update succeeded, device will reboot...");
            delay(1000);
            ESP.restart();
        } else {
            Serial.println("HTTP update failed, continuing normal operation");
            otaRequested = false;
            return;
        }
    }
    
    // Fall back to ArduinoOTA mode
    Serial.println("Mode: ArduinoOTA (Network Upload)");
    Serial.println("Waiting for OTA update...");
    Serial.println("Use PlatformIO or Arduino IDE to upload");
    Serial.println("Hostname: " + config.mqttClientID);
    
    // Clear the persistent trigger for ArduinoOTA mode too
    clearOTATrigger();
    Serial.println("Cleared OTA trigger before ArduinoOTA mode");
    
    // Wait for OTA update with timeout
    const unsigned long otaTimeout = 60000;  // 1 minute
    unsigned long otaStartTime = millis();
    
    while (millis() - otaStartTime < otaTimeout) {
        ArduinoOTA.handle();
        delay(100);
        
        // Print countdown every 10 seconds
        if ((millis() - otaStartTime) % 10000 < 100) {
            unsigned long remaining = (otaTimeout - (millis() - otaStartTime)) / 1000;
            Serial.print("Time remaining: ");
            Serial.print(remaining);
            Serial.println(" seconds");
        }
    }
    
    Serial.println("\nOTA timeout reached. Resuming normal operation.");
    otaRequested = false;
    // No need to clear again - already cleared at start of ArduinoOTA mode
}

void OTAManager::loop() {
    if (otaRequested) {
        handleUpdate();
    }
}

bool OTAManager::performHTTPUpdate(const String& filename) {
    // Construct full URL from base + filename
    String fullUrl = String(OTA_BASE_URL) + filename;
    
    Serial.println("╔══════════════════════════════════╗");
    Serial.println("║  HTTP OTA Update from GitHub     ║");
    Serial.println("╚══════════════════════════════════╝");
    Serial.print("Base URL: ");
    Serial.println(OTA_BASE_URL);
    Serial.print("Filename: ");
    Serial.println(filename);
    Serial.print("Full URL: ");
    Serial.println(fullUrl);
    Serial.println();
    
    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate verification (GitHub certs change frequently)
    
    // Configure HTTPUpdate for GitHub redirects
    httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    httpUpdate.setLedPin(LED_BUILTIN, LOW);
    
    // Add callbacks for update progress
    httpUpdate.onStart([]() {
        Serial.println("HTTP Update Started...");
    });
    
    httpUpdate.onEnd([]() {
        Serial.println("\nHTTP Update Complete!");
    });
    
    httpUpdate.onProgress([](int current, int total) {
        Serial.printf("Progress: %d%%\r", (current * 100) / total);
    });
    
    httpUpdate.onError([](int error) {
        Serial.printf("\nHTTP Update Error (%d): ", error);
        switch(error) {
            case HTTP_UPDATE_FAILED:
                Serial.println("Update failed");
                break;
            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("No updates available");
                break;
            case HTTP_UPDATE_OK:
                Serial.println("Update OK (shouldn't see this as error)");
                break;
        }
    });
    
    // Perform the update
    t_httpUpdate_return ret = httpUpdate.update(client, fullUrl);
    
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("Update failed: %s\n", httpUpdate.getLastErrorString().c_str());
            Serial.printf("HTTP error code: %d\n", httpUpdate.getLastError());
            return false;
            
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("No updates available (same version already installed)");
            return false;
            
        case HTTP_UPDATE_OK:
            Serial.println("Update successful! Rebooting...");
            return true;
    }
    
    return false;
}

bool OTAManager::isNewerVersion(const String& latestVersion, const String& currentVersion) {
    if (latestVersion.length() == 0 || currentVersion.length() == 0) {
        return false;
    }
    
    // Simple version comparison (assumes semantic versioning: X.Y.Z)
    // Parse versions
    int latestMajor = 0, latestMinor = 0, latestPatch = 0;
    int currentMajor = 0, currentMinor = 0, currentPatch = 0;
    
    sscanf(latestVersion.c_str(), "%d.%d.%d", &latestMajor, &latestMinor, &latestPatch);
    sscanf(currentVersion.c_str(), "%d.%d.%d", &currentMajor, &currentMinor, &currentPatch);
    
    // Compare versions
    if (latestMajor > currentMajor) return true;
    if (latestMajor == currentMajor && latestMinor > currentMinor) return true;
    if (latestMajor == currentMajor && latestMinor == currentMinor && latestPatch > currentPatch) return true;
    
    return false;
}

bool OTAManager::checkForUpdates() {
    Serial.println("\n╔═══════════════════════════════╗");
    Serial.println("║  Checking for OTA Updates     ║");
    Serial.println("╚═══════════════════════════════╝");
    
    String currentVersion = String(FIRMWARE_VERSION);
    Serial.print("Current version: ");
    Serial.println(currentVersion);
    
    String targetVersion = config.otaTargetVersion;
    
    if (targetVersion.length() == 0) {
        Serial.println("No target OTA version configured");
        return false;
    }
    
    Serial.print("Target version: ");
    Serial.println(targetVersion);
    
    if (isNewerVersion(targetVersion, currentVersion)) {
        Serial.println("\n✓ New version available!");
        Serial.print("  Current: ");
        Serial.println(currentVersion);
        Serial.print("  Target:  ");
        Serial.println(targetVersion);
        
        // Construct firmware filename based on battery type (version in URL path)
        #if BATTERY_TYPE == LEAD_ACID
            String batteryType = "leadacid";
        #elif BATTERY_TYPE == LIFEPO4
            String batteryType = "lifepo4";
        #else
            String batteryType = "unknown";
        #endif
        
        String firmwareFilename = "v" + targetVersion + "/firmware-" + batteryType + ".bin";
        
        Serial.print("Triggering update to: ");
        Serial.println(firmwareFilename);
        
        requestUpdate(firmwareFilename);
        return true;
    } else {
        Serial.println("✓ Firmware is up to date");
        return false;
    }
}

