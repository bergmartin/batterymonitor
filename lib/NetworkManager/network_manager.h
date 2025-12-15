#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "battery_monitor.h"
#include "battery_config.h"
#include "config_manager.h"

class NetworkManager {
private:
    WiFiClient& wifiClient;
    PubSubClient& mqttClient;
    ConfigManager& config;
    
    // Callback function pointers
    std::function<void(const String&)> otaCallback;
    std::function<void()> resetCallback;
    
public:
    bool wifiConnected;
    bool mqttConnected;
    
    NetworkManager(WiFiClient& wifi, PubSubClient& mqtt, ConfigManager& cfg)
        : wifiClient(wifi), mqttClient(mqtt), config(cfg), 
          wifiConnected(false), mqttConnected(false) {
        mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->mqttCallback(topic, payload, length);
        });
    }
    
    void setOTACallback(std::function<void(const String&)> callback) {
        otaCallback = callback;
    }
    
    void setResetCallback(std::function<void()> callback) {
        resetCallback = callback;
    }
    
    bool connectWiFi() {
        Serial.print("Connecting to WiFi: ");
        Serial.println(config.wifiSSID);
        
        WiFi.begin(config.wifiSSID.c_str(), config.wifiPassword.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < Config::WIFI_TIMEOUT_MS) {
            delay(500);
            Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(" Connected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            wifiConnected = true;
            return true;
        } else {
            Serial.println(" Failed!");
            Serial.print("WiFi connection error. Status: ");
            Serial.println(WiFi.status());
            wifiConnected = false;
            return false;
        }
    }
    
    bool connectMQTT() {
        Serial.print("Connecting to MQTT broker: ");
        Serial.println(config.mqttServer);
        mqttClient.setServer(config.mqttServer.c_str(), config.mqttPort);
        
        unsigned long startTime = millis();
        while (!mqttClient.connected() && millis() - startTime < Config::MQTT_TIMEOUT_MS) {
            if (mqttClient.connect(config.mqttClientID.c_str(), config.mqttUser.c_str(), config.mqttPassword.c_str())) {
                Serial.println(" Connected!");
                
                // Subscribe to OTA trigger topic
                char otaTopic[100];
                snprintf(otaTopic, sizeof(otaTopic), "%s/ota", Config::MQTT_TOPIC_BASE);
                mqttClient.subscribe(otaTopic);
                Serial.print("Subscribed to OTA topic: ");
                Serial.println(otaTopic);
                
                // Subscribe to reset topic
                char resetTopic[100];
                snprintf(resetTopic, sizeof(resetTopic), "%s/reset", Config::MQTT_TOPIC_BASE);
                mqttClient.subscribe(resetTopic);
                Serial.print("Subscribed to reset topic: ");
                Serial.println(resetTopic);
                
                mqttConnected = true;
                return true;
            }
            delay(500);
            Serial.print(".");
        }
        
        Serial.println(" Failed!");
        mqttConnected = false;
        return false;
    }
    
    void publishReading(const BatteryReading& reading, int bootCount) {
        if (!mqttClient.connected()) {
            Serial.println("MQTT not connected, skipping publish");
            return;
        }
        
        // Publish individual values
        char topic[100];
        char value[20];
        
        // Voltage
        snprintf(topic, sizeof(topic), "%s/voltage", Config::MQTT_TOPIC_BASE);
        snprintf(value, sizeof(value), "%.2f", reading.voltage);
        mqttClient.publish(topic, value, true);  // retained
        
        // Percentage
        snprintf(topic, sizeof(topic), "%s/percentage", Config::MQTT_TOPIC_BASE);
        snprintf(value, sizeof(value), "%.1f", reading.percentage);
        mqttClient.publish(topic, value, true);
        
        // Status as text
        snprintf(topic, sizeof(topic), "%s/status", Config::MQTT_TOPIC_BASE);
        const char* statusStr = "";
        switch(reading.status) {
            case BatteryStatus::FULL: statusStr = "FULL"; break;
            case BatteryStatus::GOOD: statusStr = "GOOD"; break;
            case BatteryStatus::LOW_BATTERY: statusStr = "LOW"; break;
            case BatteryStatus::CRITICAL: statusStr = "CRITICAL"; break;
            case BatteryStatus::DEAD: statusStr = "DEAD"; break;
        }
        mqttClient.publish(topic, statusStr, true);
        
        // Battery type
        snprintf(topic, sizeof(topic), "%s/type", Config::MQTT_TOPIC_BASE);
        mqttClient.publish(topic, Config::BATTERY_TYPE_NAME, true);
        
        // Boot count
        snprintf(topic, sizeof(topic), "%s/boot_count", Config::MQTT_TOPIC_BASE);
        snprintf(value, sizeof(value), "%d", bootCount);
        mqttClient.publish(topic, value, true);
        
        // JSON payload with all data
        snprintf(topic, sizeof(topic), "%s/json", Config::MQTT_TOPIC_BASE);
        char json[300];
        snprintf(json, sizeof(json), 
            "{\"voltage\":%.2f,\"percentage\":%.1f,\"status\":\"%s\",\"type\":\"%s\",\"boot\":%d,\"rssi\":%d}",
            reading.voltage, reading.percentage, statusStr, Config::BATTERY_TYPE_NAME, bootCount, WiFi.RSSI());
        mqttClient.publish(topic, json, true);
        
        Serial.println("Published to MQTT:");
        Serial.print("  Topic: ");
        Serial.println(Config::MQTT_TOPIC_BASE);
        Serial.print("  JSON: ");
        Serial.println(json);
    }
    
    void loop() {
        mqttClient.loop();
    }
    
    void disconnect() {
        mqttClient.disconnect();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        wifiConnected = false;
        mqttConnected = false;
    }
    
private:
    void mqttCallback(char* topic, byte* payload, unsigned int length) {
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");
        
        // Convert payload to string
        String message = "";
        for (unsigned int i = 0; i < length; i++) {
            message += (char)payload[i];
        }
        Serial.println(message);
        
        // Check if this is an OTA trigger
        String topicStr = String(topic);
        if (topicStr.endsWith("/ota")) {
            Serial.println("OTA update requested!");
            
            // Security: Only accept filenames, not full URLs or paths
            message.trim();
            if (message.indexOf('/') == -1 && message.indexOf('\\') == -1 && 
                message.indexOf(':') == -1 && message.length() > 0) {
                Serial.print("OTA Filename: ");
                Serial.println(message);
                if (otaCallback) {
                    otaCallback(message);
                }
            } else if (message.length() == 0 || message.equalsIgnoreCase("update") || message.equalsIgnoreCase("ota")) {
                // Empty message or generic trigger = use ArduinoOTA mode
                Serial.println("Mode: ArduinoOTA (no filename provided)");
                if (otaCallback) {
                    otaCallback("");
                }
            } else {
                Serial.println("ERROR: Invalid filename. Must be filename only, no paths or URLs.");
            }
        }
        
        // Check for reset NVS command
        if (topicStr.endsWith("/reset")) {
            if (message.equalsIgnoreCase("nvs") || message.equalsIgnoreCase("config")) {
                Serial.println("\n╔═══════════════════════════════╗");
                Serial.println("║   NVS Reset via MQTT          ║");
                Serial.println("╚═══════════════════════════════╝");
                if (resetCallback) {
                    resetCallback();
                }
            }
        }
    }
};

#endif // NETWORK_MANAGER_H
