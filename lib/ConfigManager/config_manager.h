#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
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
    uint16_t mqttPort;
    String mqttUser;
    String mqttPassword;
    String mqttClientID;
    
    // Deep sleep setting
    bool deepSleepEnabled;
    
    ConfigManager() : mqttPort(1883), deepSleepEnabled(true) {}
    
    void begin(const char* wifiSsidDefault, const char* wifiPassDefault,
               const char* mqttServerDefault, uint16_t mqttPortDefault,
               const char* mqttUserDefault, const char* mqttPassDefault,
               const char* mqttClientIDDefault) {
        
        preferences.begin("battery-mon", false);
        
        // Check if this is first run
        bool isFirstRun = !preferences.getBool("initialized", false);
        
        if (isFirstRun) {
            Serial.println("First run detected - initializing NVS with defaults");
            
            // Save defaults to NVS
            preferences.putString("wifi_ssid", wifiSsidDefault);
            preferences.putString("wifi_pass", wifiPassDefault);
            preferences.putString("mqtt_srv", mqttServerDefault);
            preferences.putUShort("mqtt_port", mqttPortDefault);
            preferences.putString("mqtt_user", mqttUserDefault);
            preferences.putString("mqtt_pass", mqttPassDefault);
            preferences.putString("mqtt_id", mqttClientIDDefault);
            preferences.putBool("deep_sleep", true);
            preferences.putBool("initialized", true);
            
            Serial.println("Default credentials saved to NVS");
        }
        
        // Load credentials from NVS
        wifiSSID = preferences.getString("wifi_ssid", wifiSsidDefault);
        wifiPassword = preferences.getString("wifi_pass", wifiPassDefault);
        mqttServer = preferences.getString("mqtt_srv", mqttServerDefault);
        mqttPort = preferences.getUShort("mqtt_port", mqttPortDefault);
        mqttUser = preferences.getString("mqtt_user", mqttUserDefault);
        mqttPassword = preferences.getString("mqtt_pass", mqttPassDefault);
        mqttClientID = preferences.getString("mqtt_id", mqttClientIDDefault);
        deepSleepEnabled = preferences.getBool("deep_sleep", true);
        
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║   Configuration Loaded from NVS       ║");
        Serial.println("╚═══════════════════════════════════════╝");
        Serial.print("WiFi SSID: ");
        Serial.println(wifiSSID);
        Serial.print("MQTT Server: ");
        Serial.println(mqttServer);
        Serial.print("MQTT Port: ");
        Serial.println(mqttPort);
        Serial.print("MQTT Client ID: ");
        Serial.println(mqttClientID);
        Serial.println();
    }
    
    void saveConfig() {
        preferences.putString("wifi_ssid", wifiSSID);
        preferences.putString("wifi_pass", wifiPassword);
        preferences.putString("mqtt_srv", mqttServer);
        preferences.putUShort("mqtt_port", mqttPort);
        preferences.putString("mqtt_user", mqttUser);
        preferences.putString("mqtt_pass", mqttPassword);
        preferences.putString("mqtt_id", mqttClientID);
        preferences.putBool("deep_sleep", deepSleepEnabled);
        
        Serial.println("Configuration saved to NVS");
    }
    
    void resetToDefaults(const char* wifiSsidDefault, const char* wifiPassDefault,
                        const char* mqttServerDefault, uint16_t mqttPortDefault,
                        const char* mqttUserDefault, const char* mqttPassDefault,
                        const char* mqttClientIDDefault) {
        
        wifiSSID = wifiSsidDefault;
        wifiPassword = wifiPassDefault;
        mqttServer = mqttServerDefault;
        mqttPort = mqttPortDefault;
        mqttUser = mqttUserDefault;
        mqttPassword = mqttPassDefault;
        mqttClientID = mqttClientIDDefault;
        
        saveConfig();
        Serial.println("Configuration reset to defaults");
    }
    
    void clear() {
        preferences.clear();
        preferences.putBool("initialized", false);
        Serial.println("NVS cleared - will reinitialize on next boot");
    }
    
    void printConfig() {
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║   Current Configuration              ║");
        Serial.println("╚═══════════════════════════════════════╝");
        Serial.print("WiFi SSID: ");
        Serial.println(wifiSSID);
        Serial.print("WiFi Password: ");
        Serial.println(wifiPassword.length() > 0 ? "********" : "(empty)");
        Serial.print("MQTT Server: ");
        Serial.println(mqttServer);
        Serial.print("MQTT Port: ");
        Serial.println(mqttPort);
        Serial.print("MQTT User: ");
        Serial.println(mqttUser);
        Serial.print("MQTT Password: ");
        Serial.println(mqttPassword.length() > 0 ? "********" : "(empty)");
        Serial.print("MQTT Client ID: ");
        Serial.println(mqttClientID);
        Serial.print("Deep Sleep: ");
        Serial.println(deepSleepEnabled ? "Enabled" : "Disabled");
        Serial.println();
    }
    
    void end() {
        preferences.end();
    }
};

#endif
