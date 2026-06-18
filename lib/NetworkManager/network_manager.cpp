#include "network_manager.h"
#include <time.h>
#include "../../include/mqtt_credentials.h"
#include "display_manager.h"
#include "command_handler.h"
#include <WebServer.h>
#include <WiFiAP.h>

NetworkManager::NetworkManager(WiFiClientSecure& wifi, PubSubClient& mqtt, ConfigManager& cfg)
    : wifiClient(wifi), mqttClient(mqtt), config(cfg), 
      wifiConnected(false), mqttConnected(false) {
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->mqttCallback(topic, payload, length);
    });
}

void NetworkManager::setOTACallback(std::function<void(const String&)> callback) {
    otaCallback = callback;
}

void NetworkManager::setResetCallback(std::function<void()> callback) {
    resetCallback = callback;
}

bool NetworkManager::connectWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(config.wifiSSID);
    
    // Set hostname before connecting
    WiFi.setHostname(config.mqttClientID.c_str());
    
    // Configure static IP if enabled
    if (Config::USE_STATIC_IP) {
        IPAddress staticIP, gateway, subnet, dns;
        staticIP.fromString(Config::STATIC_IP);
        gateway.fromString(Config::GATEWAY);
        subnet.fromString(Config::SUBNET);
        dns.fromString(Config::DNS);
        
        if (!WiFi.config(staticIP, gateway, subnet, dns)) {
            Serial.println("Failed to configure static IP!");
            return false;
        }
        Serial.println("Using static IP configuration");
    }
    
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
        
        // Configure timezone and NTP time sync
        // Set timezone to EST (Eastern Standard Time) - adjust based on your location
        // Format: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
        // Examples: 
        // - "CET-1CEST,M3.5.0,M10.5.0/3" for Europe/Berlin
        // - "EST5EDT,M3.2.0,M11.1.0" for America/New_York
        // - "PST8PDT,M3.2.0,M11.1.0" for America/Los_Angeles
        // - "UTC0" for UTC
        setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
        tzset();
        
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        Serial.println("NTP time sync started with timezone: EST");
        
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

bool NetworkManager::connectMQTT() {
    Serial.print("Connecting to MQTT broker: ");
    Serial.println(config.mqttServer);
    
    // Configure SSL/TLS for secure MQTT connection
    wifiClient.setCACert(MQTT_CA_CERT);
    Serial.println("SSL/TLS enabled with certificate validation");
    
    mqttClient.setServer(config.mqttServer.c_str(), config.mqttPort);
    
    // Increase buffer size for discovery messages (default 256 may be too small)
    mqttClient.setBufferSize(1024);
    Serial.printf("MQTT buffer size: %d bytes\n", mqttClient.getBufferSize());
    
    unsigned long startTime = millis();
    while (!mqttClient.connected() && millis() - startTime < Config::MQTT_TIMEOUT_MS) {
        // Prepare Last Will and Testament (LWT) for availability topic
        char stateTopic[100];
        snprintf(stateTopic, sizeof(stateTopic), "%s_availability/state", WiFi.getHostname());
        
        // Use clean_session=false to persist subscriptions across deep sleep
        // LWT: publish "offline" to availability topic when connection is lost
        if (mqttClient.connect(config.mqttClientID.c_str(), config.mqttUser.c_str(), config.mqttPassword.c_str(), 
                               stateTopic, 1, true, "offline", false)) {
            Serial.println(" Connected!");
            
            // Subscribe to OTA trigger topic with QoS 1 for guaranteed delivery
            char otaTopic[100];
            snprintf(otaTopic, sizeof(otaTopic), "%s/ota", Config::MQTT_TOPIC_BASE);
            mqttClient.subscribe(otaTopic, 1);  // QoS 1
            Serial.print("Subscribed to OTA topic (QoS 1): ");
            Serial.println(otaTopic);
            
            // Subscribe to reset topic
            char resetTopic[100];
            snprintf(resetTopic, sizeof(resetTopic), "%s/reset", Config::MQTT_TOPIC_BASE);
            mqttClient.subscribe(resetTopic, 1);  // QoS 1
            Serial.print("Subscribed to reset topic (QoS 1): ");
            Serial.println(resetTopic);
            
            // Subscribe to config change topic: battery type
            char cfgBatteryTypeTopic[128];
            snprintf(cfgBatteryTypeTopic, sizeof(cfgBatteryTypeTopic), "%s/config/battery_type", Config::MQTT_TOPIC_BASE);
            mqttClient.subscribe(cfgBatteryTypeTopic, 1);
            Serial.print("Subscribed to config topic (battery_type): ");
            Serial.println(cfgBatteryTypeTopic);
            
            // Subscribe to appliance control topic
            char applianceTopic[100];
            snprintf(applianceTopic, sizeof(applianceTopic), "%s/appliance/set", Config::MQTT_TOPIC_BASE);
            mqttClient.subscribe(applianceTopic, 1);
            Serial.print("Subscribed to appliance topic: ");
            Serial.println(applianceTopic);
            
            // Publish availability state as "online"
            char availabilityStateTopic[100];
            snprintf(availabilityStateTopic, sizeof(availabilityStateTopic), "%s_availability/state", WiFi.getHostname());
            mqttClient.publish(availabilityStateTopic, "online", true);
            Serial.print("Published availability state: online to ");
            Serial.println(availabilityStateTopic);
            
            // Publish Home Assistant discovery messages
            publishHomeAssistantDiscovery();
            
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

void NetworkManager::publishReading(const BatteryReading& reading, int bootCount, time_t nextReadingTime) {
    if (!mqttClient.connected()) {
        Serial.println("MQTT not connected, skipping publish");
        return;
    }
    
    char topic[150];
    const char* hostname = WiFi.getHostname();
    
    // Status as text
    const char* statusStr = "";
    switch(reading.status) {
        case BatteryStatus::FULL: statusStr = "FULL"; break;
        case BatteryStatus::GOOD: statusStr = "GOOD"; break;
        case BatteryStatus::LOW_BATTERY: statusStr = "LOW"; break;
        case BatteryStatus::CRITICAL: statusStr = "CRITICAL"; break;
        case BatteryStatus::DEAD: statusStr = "DEAD"; break;
    }
    
    // Publish each sensor to its own state topic
    char value[20];
    
    // Battery type
    snprintf(topic, sizeof(topic), "%s_battery_type/state", hostname);
    const char* typeName = BatteryMonitor::getBatteryTypeName();
    if (!mqttClient.publish(topic, typeName, true)) {
        Serial.printf("❌ Failed to publish battery type - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
    }
    
    // Voltage
    snprintf(topic, sizeof(topic), "%s_voltage/state", hostname);
    snprintf(value, sizeof(value), "%.2f", reading.voltage);
    if (!mqttClient.publish(topic, value, true)) {
        Serial.printf("❌ Failed to publish voltage - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
    } 
    
    // Percentage
    snprintf(topic, sizeof(topic), "%s_percentage/state", hostname);
    snprintf(value, sizeof(value), "%.1f", reading.percentage);
    if (!mqttClient.publish(topic, value, true)) {
        Serial.printf("❌ Failed to publish percentage - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
    }
    
    // Status
    snprintf(topic, sizeof(topic), "%s_status/state", hostname);
    if (!mqttClient.publish(topic, statusStr, true)) {
        Serial.printf("❌ Failed to publish status - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
    } 
    
    // RSSI
    snprintf(topic, sizeof(topic), "%s_rssi/state", hostname);
    snprintf(value, sizeof(value), "%d", WiFi.RSSI());
    if (!mqttClient.publish(topic, value, true)) {
        Serial.printf("❌ Failed to publish RSSI - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
    } 
    
    // Boot count
    snprintf(topic, sizeof(topic), "%s_boot/state", hostname);
    snprintf(value, sizeof(value), "%d", bootCount);
    if (!mqttClient.publish(topic, value, true)) {
        Serial.printf("❌ Failed to publish boot count - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
    }
    
    // Last updated (ISO 8601 timestamp)
    snprintf(topic, sizeof(topic), "%s_last_updated/state", hostname);
    time_t now;
    struct tm timeinfo;
    time(&now);
    if (getLocalTime(&timeinfo)) {
        char timestamp[30];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeinfo);
        if (!mqttClient.publish(topic, timestamp, true)) {
            Serial.printf("❌ Failed to publish last updated time - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
        }
    } else {
        // Fallback if NTP not synced yet
        snprintf(value, sizeof(value), "%lu", millis() / 1000);
        if (!mqttClient.publish(topic, value, true)) {
            Serial.printf("❌ Failed to publish last updated time (no NTP) - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
        }
    }
    
    // Next reading time (ISO 8601 timestamp)
    if (nextReadingTime > 0) {
        snprintf(topic, sizeof(topic), "%s_next_reading/state", hostname);
        struct tm nextTimeinfo;
        localtime_r(&nextReadingTime, &nextTimeinfo);
        char nextTimestamp[30];
        strftime(nextTimestamp, sizeof(nextTimestamp), "%Y-%m-%d %H:%M:%S", &nextTimeinfo);
        if (!mqttClient.publish(topic, nextTimestamp, true)) {
            Serial.printf("❌ Failed to publish next reading time - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
        }
    }
    
    // Firmware version
    snprintf(topic, sizeof(topic), "%s_firmware/state", hostname);
    const char* fwVersion = 
        #ifdef FIRMWARE_VERSION
        FIRMWARE_VERSION
        #else
        "dev"
        #endif
    ;
    if (!mqttClient.publish(topic, fwVersion, true)) {
        Serial.printf("❌ Failed to publish firmware version - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
    }

    // Appliance actual state
    snprintf(topic, sizeof(topic), "%s_appliance/state", hostname);
    bool actualState = digitalRead(Config::APPLIANCE_RELAY_PIN);
    if (!mqttClient.publish(topic, actualState ? "ON" : "OFF", true)) {
        Serial.printf("❌ Failed to publish appliance state - State: %d, Buffer: %d bytes\n", 
                      mqttClient.state(), mqttClient.getBufferSize());
    }
    
    Serial.printf("Published sensor states for device: %s\n", hostname);
}

void NetworkManager::publishHomeAssistantDiscovery() {
    Serial.println("Publishing Home Assistant MQTT Discovery...");
    
    char topic[150];
    char payload[600];
    const char* hostname = WiFi.getHostname();
    
    // Device information (shared across all sensors)
    char deviceInfo[250];
    snprintf(deviceInfo, sizeof(deviceInfo),
        "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"model\":\"Battery Monitor\",\"manufacturer\":\"ESP32\",\"sw_version\":\"%s\"}",
        hostname, hostname,
        #ifdef FIRMWARE_VERSION
        FIRMWARE_VERSION
        #else
        "dev"
        #endif
    );
    
    // Voltage sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_voltage/config", hostname);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Battery Voltage\",\"state_topic\":\"%s_voltage/state\",\"unit_of_measurement\":\"V\",\"device_class\":\"voltage\",\"state_class\":\"measurement\",\"unique_id\":\"%s_voltage\",%s}",
        hostname, hostname, deviceInfo);
    if (!mqttClient.publish(topic, payload, true)) {
        Serial.println("Failed to publish voltage sensor config");
    } 
    
    // Battery percentage sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_percentage/config", hostname);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Battery Level\",\"state_topic\":\"%s_percentage/state\",\"unit_of_measurement\":\"%%\",\"device_class\":\"battery\",\"state_class\":\"measurement\",\"unique_id\":\"%s_percentage\",%s}",
        hostname, hostname, deviceInfo);
    if (!mqttClient.publish(topic, payload, true)) {
        Serial.println("Failed to publish percentage sensor config");
    }
    
    // Status sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_status/config", hostname);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Battery Status\",\"state_topic\":\"%s_status/state\",\"icon\":\"mdi:battery-check\",\"unique_id\":\"%s_status\",%s}",
        hostname, hostname, deviceInfo);
    if (!mqttClient.publish(topic, payload, true)) {
        Serial.println("Failed to publish status sensor config");
    }
    
    // RSSI sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_rssi/config", hostname);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"WiFi Signal\",\"state_topic\":\"%s_rssi/state\",\"unit_of_measurement\":\"dBm\",\"device_class\":\"signal_strength\",\"state_class\":\"measurement\",\"unique_id\":\"%s_rssi\",%s}",
        hostname, hostname, deviceInfo);
    if (!mqttClient.publish(topic, payload, true)) {
        Serial.println("Failed to publish RSSI sensor config");
    }
    
    // Boot count sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_boot/config", hostname);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Boot Count\",\"state_topic\":\"%s_boot/state\",\"icon\":\"mdi:restart\",\"state_class\":\"total_increasing\",\"unique_id\":\"%s_boot\",%s}",
        hostname, hostname, deviceInfo);
    if (!mqttClient.publish(topic, payload, true)) {
        Serial.println("Failed to publish boot count sensor config");
    }

    // Last updated sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_last_updated/config", hostname);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Last Updated\",\"state_topic\":\"%s_last_updated/state\",\"device_class\":\"timestamp\",\"icon\":\"mdi:clock-check\",\"unique_id\":\"%s_last_updated\",%s}",
        hostname, hostname, deviceInfo);
    if (!mqttClient.publish(topic, payload, true)) {
        Serial.println("Failed to publish last updated sensor config");
    }
    
    // Firmware version sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_firmware/config", hostname);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Firmware Version\",\"state_topic\":\"%s_firmware/state\",\"icon\":\"mdi:chip\",\"entity_category\":\"diagnostic\",\"unique_id\":\"%s_firmware\",%s}",
        hostname, hostname, deviceInfo);
    if (!mqttClient.publish(topic, payload, true)) {
        Serial.println("Failed to publish firmware version sensor config");
    }
    
    // Battery type sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_battery_type/config", hostname);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Battery Type\",\"state_topic\":\"%s_battery_type/state\",\"icon\":\"mdi:battery\",\"unique_id\":\"%s_battery_type\",%s}",
        hostname, hostname, deviceInfo);
    if (!mqttClient.publish(topic, payload, true)) {
        Serial.println("Failed to publish battery type sensor config");
    }

    // Appliance Switch
    snprintf(topic, sizeof(topic), "homeassistant/switch/%s_appliance/config", hostname);
    snprintf(payload, sizeof(payload),
        "{\"name\":\"Appliance\",\"state_topic\":\"%s_appliance/state\",\"command_topic\":\"%s/appliance/set\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"unique_id\":\"%s_appliance\",%s}",
        hostname, Config::MQTT_TOPIC_BASE, hostname, deviceInfo);
    if (!mqttClient.publish(topic, payload, true)) {
        Serial.println("Failed to publish appliance switch config");
    }
    
    // Configure availability for all sensors (shared state topic)
    char availabilityConfig[250];
    snprintf(availabilityConfig, sizeof(availabilityConfig),
        ",\"availability_topic\":\"%s_availability/state\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\"",
        hostname);
    
    // Update all sensor configs with availability (done in next connection)
    Serial.println("Note: Availability topic configured for all sensors");
    Serial.printf("Availability Topic: %s_availability/state\n", hostname);
    
    Serial.println("Home Assistant discovery published");
}

void NetworkManager::loop() {
    mqttClient.loop();
}

void NetworkManager::disconnect() {
    // Publish offline state before disconnecting
    if (mqttClient.connected()) {
        char stateTopic[100];
        snprintf(stateTopic, sizeof(stateTopic), "%s_availability/state", WiFi.getHostname());
        mqttClient.publish(stateTopic, "offline", true);
        mqttClient.loop();  // Process the publish
        delay(100);  // Give time for message to send
    }
    
    mqttClient.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    wifiConnected = false;
    mqttConnected = false;
}

void NetworkManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
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
        // Security: Accept version paths or filenames, but not full URLs
        message.trim();
        
        // Ignore empty messages (these are for clearing retained flag)
        if (message.length() == 0) {
            Serial.println("Received empty OTA message (retained flag clear)");
            return;
        }
        
        Serial.println("OTA update requested!");
        
        // Clear the retained message FIRST to avoid re-triggering after reboot
        char otaTopic[100];
        snprintf(otaTopic, sizeof(otaTopic), "%s/ota", Config::MQTT_TOPIC_BASE);
        
        Serial.println("Clearing retained OTA message from broker...");
        mqttClient.publish(otaTopic, "", true);  // Clear retained message
        
        // Process outgoing publish and wait for it to complete
        // Need longer delay to ensure broker receives and processes the clear
        for (int i = 0; i < 20; i++) {
            mqttClient.loop();
            delay(50);
        }
        Serial.println("Retained OTA command cleared");
        
        if (message.indexOf('\\') == -1 && message.indexOf(':') == -1) {
            // Valid: "v1.0.2/firmware-leadacid.bin" or "firmware.bin" or "update"/"ota"
            if (message.equalsIgnoreCase("update") || message.equalsIgnoreCase("ota")) {
                // Generic trigger = use ArduinoOTA mode
                Serial.println("Mode: ArduinoOTA (network upload)");
                if (otaCallback) {
                    otaCallback("");
                }
            } else {
                // Specific firmware path/filename
                Serial.print("OTA Path/Filename: ");
                Serial.println(message);
                if (otaCallback) {
                    otaCallback(message);
                }
            }
        } else {
            Serial.println("ERROR: Invalid path/filename. Must not contain backslashes or colons.");
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

    // Handle configuration changes: battery type
    if (topicStr.endsWith("/config/battery_type")) {
        String newType = message;
        newType.trim();
        newType.toLowerCase();
        if (newType == "lifepo4" || newType == "life" || newType == "li" ) {
            config.batteryType = "lifepo4";
            BatteryMonitor::setChemistry(BatteryChemistry::LIFEPO4);
        } else if (newType == "leadacid" || newType == "lead" || newType == "sla") {
            config.batteryType = "leadacid";
            BatteryMonitor::setChemistry(BatteryChemistry::LEAD_ACID);
        } else {
            Serial.println("Invalid battery_type. Use 'leadacid' or 'lifepo4'.");
            return;
        }
        config.saveConfig();
        Serial.print("Battery type updated via MQTT: ");
        Serial.println(config.batteryType);
        // Acknowledge by publishing current type to a state topic
        char typeStateTopic[100];
        snprintf(typeStateTopic, sizeof(typeStateTopic), "%s_battery_type/state", WiFi.getHostname());
        mqttClient.publish(typeStateTopic, config.batteryType.c_str(), true);
        Serial.print("Published battery_type state: ");
        Serial.println(typeStateTopic);
    }

    // Handle appliance relay control
    if (topicStr.endsWith("/appliance/set")) {
        message.trim();
        if (message.equalsIgnoreCase("ON") || message.equalsIgnoreCase("1") || message.equalsIgnoreCase("true")) {
            config.applianceTargetState = true;
        } else if (message.equalsIgnoreCase("OFF") || message.equalsIgnoreCase("0") || message.equalsIgnoreCase("false")) {
            config.applianceTargetState = false;
        } else {
            Serial.println("Invalid appliance command. Use ON/OFF, 1/0, or true/false.");
            return;
        }
        config.saveConfig();
        Serial.print("Appliance target state updated via MQTT: ");
        Serial.println(config.applianceTargetState ? "ON" : "OFF");
    }
}

// HTML templates for the configuration portal
const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Battery Monitor Config Portal</title>
<style>
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);
  color: #f8fafc;
  margin: 0;
  padding: 20px;
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 100vh;
}
.card {
  background: #1e293b;
  border: 1px solid #334155;
  border-radius: 16px;
  box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.5);
  padding: 30px;
  width: 100%;
  max-width: 480px;
  box-sizing: border-box;
}
h2 {
  margin-top: 0;
  font-size: 24px;
  font-weight: 700;
  color: #0ea5e9;
  text-align: center;
  margin-bottom: 20px;
  border-bottom: 1px solid #334155;
  padding-bottom: 15px;
}
.group {
  margin-bottom: 18px;
}
label {
  display: block;
  font-size: 13px;
  font-weight: 600;
  color: #94a3b8;
  margin-bottom: 6px;
  text-transform: uppercase;
  letter-spacing: 0.05em;
}
input[type="text"], input[type="password"], select {
  width: 100%;
  padding: 10px 12px;
  background: #0f172a;
  border: 1px solid #475569;
  border-radius: 8px;
  color: #f8fafc;
  font-size: 14px;
  box-sizing: border-box;
  transition: all 0.2s;
}
input:focus, select:focus {
  outline: none;
  border-color: #0ea5e9;
  box-shadow: 0 0 0 3px rgba(14, 165, 233, 0.15);
}
.checkbox-group {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 5px;
}
.checkbox-group input {
  margin: 0;
  width: 16px;
  height: 16px;
  accent-color: #0ea5e9;
}
.checkbox-group label {
  margin-bottom: 0;
  text-transform: none;
  font-size: 14px;
  font-weight: 500;
  color: #e2e8f0;
}
button {
  width: 100%;
  padding: 12px;
  background: linear-gradient(135deg, #0ea5e9 0%, #0284c7 100%);
  border: none;
  border-radius: 8px;
  color: #ffffff;
  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s;
  box-shadow: 0 4px 6px -1px rgba(14, 165, 233, 0.2);
  margin-top: 15px;
}
button:hover {
  background: linear-gradient(135deg, #38bdf8 0%, #0ea5e9 100%);
  transform: translateY(-1px);
  box-shadow: 0 10px 15px -3px rgba(14, 165, 233, 0.3);
}
.footer {
  text-align: center;
  font-size: 11px;
  color: #64748b;
  margin-top: 20px;
}
</style>
</head>
<body>
<div class="card">
  <h2>Battery Monitor Config</h2>
  <form action="/save" method="POST">
    <div class="group">
      <label for="wifi_select">Available Networks</label>
      <select id="wifi_select" onchange="document.getElementById('wifi_ssid').value = this.value;">
        {WIFI_LIST}
      </select>
    </div>
    <div class="group">
      <label for="wifi_ssid">WiFi SSID</label>
      <input type="text" id="wifi_ssid" name="wifi_ssid" placeholder="Enter SSID" value="{SSID_VALUE}" required>
    </div>
    <div class="group">
      <label for="wifi_pass">WiFi Password</label>
      <input type="password" id="wifi_pass" name="wifi_pass" placeholder="Enter Password" value="">
    </div>
    <div class="group">
      <label for="mqtt_srv">MQTT Broker IP / Host</label>
      <input type="text" id="mqtt_srv" name="mqtt_srv" placeholder="e.g. 192.168.1.100" value="{MQTT_SRV_VALUE}" required>
    </div>
    <div class="group">
      <label for="mqtt_port">MQTT Port</label>
      <input type="text" id="mqtt_port" name="mqtt_port" placeholder="1883" value="{MQTT_PORT_VALUE}" required>
    </div>
    <div class="group">
      <label for="mqtt_user">MQTT Username (Optional)</label>
      <input type="text" id="mqtt_user" name="mqtt_user" placeholder="e.g. user" value="{MQTT_USER_VALUE}">
    </div>
    <div class="group">
      <label for="mqtt_pass">MQTT Password (Optional)</label>
      <input type="password" id="mqtt_pass" name="mqtt_pass" placeholder="e.g. password" value="">
    </div>
    <div class="group">
      <label for="mqtt_id">MQTT Client ID</label>
      <input type="text" id="mqtt_id" name="mqtt_id" placeholder="e.g. esp32-battery" value="{MQTT_ID_VALUE}" required>
    </div>
    <div class="group">
      <label for="battery_type">Battery Chemistry</label>
      <select id="battery_type" name="battery_type">
        {BATT_TYPE_OPTIONS}
      </select>
    </div>
    <div class="group checkbox-group">
      <input type="checkbox" id="deep_sleep" name="deep_sleep" value="true" {DEEP_SLEEP_CHECKED}>
      <label for="deep_sleep">Enable Deep Sleep (Power Saving)</label>
    </div>
    <button type="submit">Save &amp; Connect</button>
  </form>
  <div class="footer">ESP32 Battery Monitor Config Portal</div>
</div>
</body>
</html>
)rawliteral";

const char SAVED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Settings Saved</title>
<style>
body {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);
  color: #f8fafc;
  margin: 0;
  padding: 20px;
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 100vh;
}
.card {
  background: #1e293b;
  border: 1px solid #334155;
  border-radius: 16px;
  box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.5);
  padding: 40px 30px;
  width: 100%;
  max-width: 400px;
  text-align: center;
}
.icon {
  font-size: 48px;
  color: #22c55e;
  margin-bottom: 15px;
}
h2 {
  margin-top: 0;
  color: #22c55e;
  font-size: 22px;
}
p {
  color: #94a3b8;
  font-size: 15px;
  line-height: 1.5;
  margin-bottom: 25px;
}
.spinner {
  border: 3px solid rgba(14, 165, 233, 0.1);
  width: 36px;
  height: 36px;
  clear: both;
  margin: 20px auto;
  border-top-color: #0ea5e9;
  border-radius: 50%;
  animation: spin 1s infinite linear;
}
@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}
</style>
</head>
<body>
<div class="card">
  <div class="icon">&#x2714;</div>
  <h2>Configuration Saved!</h2>
  <p>Settings saved successfully. The device is restarting now to connect to the new WiFi network...</p>
  <div class="spinner"></div>
</div>
</body>
</html>
)rawliteral";

String NetworkManager::escapeHtml(const String& str) {
    String escaped = "";
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        switch (c) {
            case '&':  escaped += "&amp;"; break;
            case '<':  escaped += "&lt;"; break;
            case '>':  escaped += "&gt;"; break;
            case '"':  escaped += "&quot;"; break;
            case '\'': escaped += "&#x27;"; break;
            case '/':  escaped += "&#x2F;"; break;
            default:   escaped += c; break;
        }
    }
    return escaped;
}

void NetworkManager::startAPMode(const BatteryReading& reading, DisplayManager& display, CommandHandler& commandHandler) {
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║   Entering WiFi Config AP Mode        ║");
    Serial.println("╚═══════════════════════════════════════╝");
    
    // 1. Disconnect current WiFi connection
    WiFi.disconnect(true);
    delay(500);
    
    // 2. Scan networks
    Serial.println("Scanning local WiFi networks...");
    int n = WiFi.scanNetworks();
    Serial.printf("Found %d networks.\n", n);
    
    String wifiList = "";
    if (n <= 0) {
        wifiList = "<option value=\"\">No networks found</option>";
    } else {
        wifiList = "<option value=\"\">-- Select a network --</option>";
        for (int i = 0; i < n; ++i) {
            String ssid = WiFi.SSID(i);
            int32_t rssi = WiFi.RSSI(i);
            String escaped = escapeHtml(ssid);
            wifiList += "<option value=\"" + escaped + "\">" + escaped + " (" + String(rssi) + " dBm)</option>";
        }
    }
    
    // 3. Generate SSID & start softAP
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apSsid[32];
    snprintf(apSsid, sizeof(apSsid), "%s%02X%02X", Config::AP_SSID_PREFIX, mac[4], mac[5]);
    
    WiFi.mode(WIFI_AP);
    bool apStarted = false;
    if (strlen(Config::AP_PASSWORD) > 0) {
        apStarted = WiFi.softAP(apSsid, Config::AP_PASSWORD);
    } else {
        apStarted = WiFi.softAP(apSsid);
    }
    
    if (!apStarted) {
        Serial.println("Failed to start softAP!");
        return;
    }
    
    IPAddress apIP = WiFi.softAPIP();
    Serial.print("Access Point started successfully.\nSSID: ");
    Serial.println(apSsid);
    Serial.print("IP Address: ");
    Serial.println(apIP);
    
    // 4. Start Web Server
    WebServer server(80);
    bool rebootRequested = false;
    
    server.on("/", HTTP_GET, [&]() {
        String page = String(PORTAL_HTML);
        page.replace("{WIFI_LIST}", wifiList);
        page.replace("{SSID_VALUE}", escapeHtml(config.wifiSSID));
        page.replace("{MQTT_SRV_VALUE}", escapeHtml(config.mqttServer));
        page.replace("{MQTT_PORT_VALUE}", String(config.mqttPort));
        page.replace("{MQTT_USER_VALUE}", escapeHtml(config.mqttUser));
        page.replace("{MQTT_ID_VALUE}", escapeHtml(config.mqttClientID));
        
        String batteryOptions = "";
        if (config.batteryType.equalsIgnoreCase("lifepo4")) {
            batteryOptions = "<option value=\"lifepo4\" selected>LiFePO4</option><option value=\"leadacid\">Lead-Acid</option>";
        } else {
            batteryOptions = "<option value=\"lifepo4\">LiFePO4</option><option value=\"leadacid\" selected>Lead-Acid</option>";
        }
        page.replace("{BATT_TYPE_OPTIONS}", batteryOptions);
        page.replace("{DEEP_SLEEP_CHECKED}", config.deepSleepEnabled ? "checked" : "");
        
        server.send(200, "text/html", page);
    });
    
    server.on("/save", HTTP_POST, [&]() {
        if (server.hasArg("wifi_ssid")) {
            config.wifiSSID = server.arg("wifi_ssid");
        }
        if (server.hasArg("wifi_pass") && server.arg("wifi_pass").length() > 0) {
            config.wifiPassword = server.arg("wifi_pass");
        }
        if (server.hasArg("mqtt_srv")) {
            config.mqttServer = server.arg("mqtt_srv");
        }
        if (server.hasArg("mqtt_port")) {
            config.mqttPort = server.arg("mqtt_port").toInt();
        }
        if (server.hasArg("mqtt_user")) {
            config.mqttUser = server.arg("mqtt_user");
        }
        if (server.hasArg("mqtt_pass") && server.arg("mqtt_pass").length() > 0) {
            config.mqttPassword = server.arg("mqtt_pass");
        }
        if (server.hasArg("mqtt_id")) {
            config.mqttClientID = server.arg("mqtt_id");
        }
        if (server.hasArg("battery_type")) {
            config.batteryType = server.arg("battery_type");
        }
        config.deepSleepEnabled = server.hasArg("deep_sleep") && (server.arg("deep_sleep") == "true" || server.arg("deep_sleep") == "on");
        
        config.saveConfig();
        
        server.send(200, "text/html", String(SAVED_HTML));
        rebootRequested = true;
    });
    
    server.begin();
    Serial.println("Web server started on port 80.");
    
    // 5. Handle portal loop with timeout
    unsigned long startMs = millis();
    unsigned long lastUpdateMs = 0;
    
    while (!rebootRequested) {
        unsigned long now = millis();
        unsigned long elapsedS = (now - startMs) / 1000;
        
        if (elapsedS >= Config::AP_TIMEOUT_S) {
            Serial.println("AP mode timed out. Restarting...");
            break;
        }
        
        server.handleClient();
        commandHandler.checkCommands();
        
        if (now - lastUpdateMs >= 1000) {
            lastUpdateMs = now;
            unsigned long remainingS = Config::AP_TIMEOUT_S - elapsedS;
            
            // Show AP information on SH1106 Display
            display.showAPScreen(apSsid, apIP.toString().c_str(), reading, remainingS);
            
            // Print progress via serial
            Serial.printf("AP Mode active... %lu s remaining.\n", remainingS);
        }
        
        delay(20);
    }
    
    // 6. Cleanup
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    
    Serial.println("Access Point stopped.");
    
    if (rebootRequested) {
        Serial.println("Rebooting in 2 seconds...");
        delay(2000);
        ESP.restart();
    }
}
