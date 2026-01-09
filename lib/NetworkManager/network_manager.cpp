#include "network_manager.h"
#include <time.h>

NetworkManager::NetworkManager(WiFiClient& wifi, PubSubClient& mqtt, ConfigManager& cfg)
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
            
            // Publish availability state as "online"
            char stateTopic[100];
            snprintf(stateTopic, sizeof(stateTopic), "%s_availability/state", WiFi.getHostname());
            mqttClient.publish(stateTopic, "online", true);
            Serial.print("Published availability state: online to ");
            Serial.println(stateTopic);
            
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
}
