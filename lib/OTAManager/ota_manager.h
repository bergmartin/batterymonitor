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
    
public:
    OTAManager(ConfigManager& cfg) : config(cfg), otaRequested(false), otaFilename("") {}
    
    void setup() {
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
    
    void requestUpdate(const String& filename) {
        otaRequested = true;
        otaFilename = filename;
        Serial.print("OTA requested with filename: ");
        Serial.println(filename.length() > 0 ? filename : "(ArduinoOTA mode)");
    }
    
    bool isUpdateRequested() const {
        return otaRequested;
    }
    
    void handleUpdate() {
        if (!otaRequested) return;
        
        Serial.println("\n╔═══════════════════════════════╗");
        Serial.println("║   Entering OTA Mode           ║");
        Serial.println("╚═══════════════════════════════╝");
        
        // Check if filename is provided for HTTP update
        if (otaFilename.length() > 0) {
            Serial.println("Mode: HTTP Update from GitHub");
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
    }
    
    void loop() {
        if (otaRequested) {
            handleUpdate();
        }
    }
    
private:
    bool performHTTPUpdate(const String& filename) {
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
        
        WiFiClient client;
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
                return false;
                
            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("No updates available");
                return false;
                
            case HTTP_UPDATE_OK:
                Serial.println("Update successful! Rebooting...");
                return true;
        }
        
        return false;
    }
};

#endif // OTA_MANAGER_H
