#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>
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
    
    void mqttCallback(char* topic, byte* payload, unsigned int length);
    
public:
    bool wifiConnected;
    bool mqttConnected;
    
    NetworkManager(WiFiClient& wifi, PubSubClient& mqtt, ConfigManager& cfg);
    
    void setOTACallback(std::function<void(const String&)> callback);
    void setResetCallback(std::function<void()> callback);
    bool connectWiFi();
    bool connectMQTT();
    void publishReading(const BatteryReading& reading, int bootCount);
    void loop();
    void disconnect();
};

#endif // NETWORK_MANAGER_H
