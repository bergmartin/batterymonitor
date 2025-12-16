#include "network_manager.h"

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
    
    unsigned long startTime = millis();
    while (!mqttClient.connected() && millis() - startTime < Config::MQTT_TIMEOUT_MS) {
        // Use clean_session=false to persist subscriptions across deep sleep
        if (mqttClient.connect(config.mqttClientID.c_str(), config.mqttUser.c_str(), config.mqttPassword.c_str(), 
                               nullptr, 0, false, nullptr, false)) {
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

void NetworkManager::publishReading(const BatteryReading& reading, int bootCount) {
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
    
    // Hostname
    snprintf(topic, sizeof(topic), "%s/hostname", Config::MQTT_TOPIC_BASE);
    mqttClient.publish(topic, WiFi.getHostname(), true);
    
    // Firmware version
    snprintf(topic, sizeof(topic), "%s/version", Config::MQTT_TOPIC_BASE);
    #ifdef FIRMWARE_VERSION
    mqttClient.publish(topic, FIRMWARE_VERSION, true);
    #else
    mqttClient.publish(topic, "unknown", true);
    #endif
    
    // JSON payload with all data
    snprintf(topic, sizeof(topic), "%s/json", Config::MQTT_TOPIC_BASE);
    char json[400];
    snprintf(json, sizeof(json), 
        "{\"voltage\":%.2f,\"percentage\":%.1f,\"status\":\"%s\",\"type\":\"%s\",\"boot\":%d,\"rssi\":%d,\"hostname\":\"%s\",\"version\":\"%s\"}",
        reading.voltage, reading.percentage, statusStr, Config::BATTERY_TYPE_NAME, bootCount, WiFi.RSSI(), 
        WiFi.getHostname(), 
        #ifdef FIRMWARE_VERSION
        FIRMWARE_VERSION
        #else
        "unknown"
        #endif
    );
    mqttClient.publish(topic, json, true);
    
    Serial.println("Published to MQTT:");
    Serial.print("  Topic: ");
    Serial.println(Config::MQTT_TOPIC_BASE);
    Serial.print("  JSON: ");
    Serial.println(json);
}

void NetworkManager::loop() {
    mqttClient.loop();
}

void NetworkManager::disconnect() {
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
}
