/*
 * ESP32 Battery Voltage Monitor with Deep Sleep & MQTT
 * 
 * Monitors a 12V battery using a voltage divider circuit
 * Supports Lead-Acid and LiFePO4 battery types
 * Features deep sleep mode for ultra-low power consumption
 * Publishes readings to MQTT broker
 * 
 * Hardware Setup:
 * - Battery + terminal -> R1 (30kΩ) -> ADC Pin (GPIO34) -> R2 (10kΩ) -> GND
 * - This creates a 4:1 voltage divider (12V becomes ~3V at ADC)
 * 
 * Power Consumption:
 * - Active: ~80-160 mA (during reading, ~5 seconds)
 * - Deep Sleep: ~0.01 mA (between readings, 4 hours)
 * - Average: ~0.1-0.3 mA (>99.9% time in deep sleep)
 * 
 * Battery Type: Set in platformio.ini with -D BATTERY_TYPE=LEAD_ACID or LIFEPO4
 * WiFi & MQTT: Configure in wifi_credentials.h and mqtt_credentials.h
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include "battery_monitor.h"
#include "esp_sleep.h"
#include "config_manager.h"

// Define LED_BUILTIN if not already defined (some ESP32 boards don't have it)
#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // GPIO2 is commonly used for built-in LED on ESP32
#endif

// Include credentials (create these files!)
// These are now used as DEFAULT VALUES only - actual credentials stored in NVS
#include "wifi_credentials.h"  // Defines WIFI_SSID and WIFI_PASSWORD
#include "mqtt_credentials.h"  // Defines MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_CLIENT_ID

// OTA base URL (set at compile time for security)
#ifndef OTA_BASE_URL
#define OTA_BASE_URL "https://github.com/USERNAME/REPO/releases/download/"
#endif

// RTC memory to preserve data across deep sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR float lastVoltage = 0.0;

// Global objects
BatteryMonitor monitor;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
ConfigManager config;  // Manages credentials in NVS (persists across OTA updates)

// Connection status flags
bool wifiConnected = false;
bool mqttConnected = false;
bool otaRequested = false;  // Flag to trigger OTA mode
String otaFilename = "";  // Filename for HTTP OTA update (base URL is compile-time constant)

void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Wakeup caused by touchpad");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("Wakeup caused by ULP program");
      break;
    default:
      Serial.println("Wakeup was not caused by deep sleep (first boot)");
      break;
  }
}

bool connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(config.wifiSSID);
  
  // WiFi.mode(WIFI_STA);
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
    return true;
  } else {
    Serial.println(" Failed!");
    Serial.print("WiFi connection error. Status: ");
    Serial.println(WiFi.status());
    return false;
  }
}

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
    otaRequested = true;
    
    // Security: Only accept filenames, not full URLs or paths
    message.trim();
    if (message.indexOf('/') == -1 && message.indexOf('\\') == -1 && 
        message.indexOf(':') == -1 && message.length() > 0) {
      otaFilename = message;
      Serial.print("OTA Filename: ");
      Serial.println(otaFilename);
      Serial.print("Base URL: ");
      Serial.println(OTA_BASE_URL);
    } else if (message.length() == 0 || message.equalsIgnoreCase("update") || message.equalsIgnoreCase("ota")) {
      // Empty message or generic trigger = use ArduinoOTA mode
      otaFilename = "";
      Serial.println("Mode: ArduinoOTA (no filename provided)");
    } else {
      Serial.println("ERROR: Invalid filename. Must be filename only, no paths or URLs.");
      otaRequested = false;
    }
  }
  
  // Check for reset NVS command
  if (topicStr.endsWith("/reset")) {
    if (message.equalsIgnoreCase("nvs") || message.equalsIgnoreCase("config")) {
      Serial.println("\n╔═══════════════════════════════╗");
      Serial.println("║   NVS Reset via MQTT          ║");
      Serial.println("╚═══════════════════════════════╝");
      config.clear();
      Serial.println("NVS will be cleared. Rebooting in 2 seconds...");
      delay(2000);
      ESP.restart();
    }
  }
}

bool connectMQTT() {
  Serial.print("Connecting to MQTT broker: ");
  Serial.println(config.mqttServer);
  mqttClient.setServer(config.mqttServer.c_str(), config.mqttPort);
  mqttClient.setCallback(mqttCallback);
  
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
      
      return true;
    }
    delay(500);
    Serial.print(".");
  }
  
  Serial.println(" Failed!");
  return false;
}

void publishToMQTT(const BatteryReading& reading) {
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

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Parse command and arguments
    String cmd = command;
    String arg = "";
    int spaceIndex = command.indexOf(' ');
    if (spaceIndex > 0) {
      cmd = command.substring(0, spaceIndex);
      arg = command.substring(spaceIndex + 1);
      arg.trim();
    }
    cmd.toLowerCase();
    
    if (cmd == "reset" && (arg == "nvs" || arg == "")) {
      Serial.println("\n╔═══════════════════════════════╗");
      Serial.println("║   Clearing NVS Storage        ║");
      Serial.println("╚═══════════════════════════════╝");
      config.clear();
      Serial.println("NVS cleared. Rebooting...");
      delay(1000);
      ESP.restart();
    }
    else if (cmd == "show" || cmd == "config") {
      config.printConfig();
    }
    else if (cmd == "save") {
      config.saveConfig();
      Serial.println("✓ Configuration saved to NVS");
    }
    else if (cmd == "set") {
      // Parse "set key value" commands
      int secondSpace = arg.indexOf(' ');
      if (secondSpace > 0) {
        String key = arg.substring(0, secondSpace);
        String value = arg.substring(secondSpace + 1);
        key.trim();
        key.toLowerCase();
        value.trim();
        
        bool validKey = true;
        
        if (key == "wifi_ssid" || key == "ssid") {
          config.wifiSSID = value;
          Serial.print("✓ WiFi SSID set to: ");
          Serial.println(value);
        }
        else if (key == "wifi_password" || key == "wifi_pass" || key == "password") {
          config.wifiPassword = value;
          Serial.println("✓ WiFi password set (hidden)");
        }
        else if (key == "mqtt_server" || key == "server") {
          config.mqttServer = value;
          Serial.print("✓ MQTT server set to: ");
          Serial.println(value);
        }
        else if (key == "mqtt_port" || key == "port") {
          config.mqttPort = value.toInt();
          Serial.print("✓ MQTT port set to: ");
          Serial.println(config.mqttPort);
        }
        else if (key == "mqtt_user" || key == "user") {
          config.mqttUser = value;
          Serial.print("✓ MQTT user set to: ");
          Serial.println(value);
        }
        else if (key == "mqtt_password" || key == "mqtt_pass") {
          config.mqttPassword = value;
          Serial.println("✓ MQTT password set (hidden)");
        }
        else if (key == "mqtt_client_id" || key == "client_id" || key == "id") {
          config.mqttClientID = value;
          Serial.print("✓ MQTT client ID set to: ");
          Serial.println(value);
        }
        else if (key == "deep_sleep") {
          value.toLowerCase();
          if (value == "true" || value == "1" || value == "on" || value == "enable") {
            config.deepSleepEnabled = true;
            Serial.println("✓ Deep sleep enabled");
          } else if (value == "false" || value == "0" || value == "off" || value == "disable") {
            config.deepSleepEnabled = false;
            Serial.println("✓ Deep sleep disabled");
          } else {
            validKey = false;
            Serial.print("✗ Invalid value: ");
            Serial.println(value);
            Serial.println("Use: true/false, on/off, enable/disable, or 1/0");
          }
        }
        else {
          validKey = false;
          Serial.print("✗ Unknown key: ");
          Serial.println(key);
          Serial.println("Type 'help' for valid keys");
        }
        
        if (validKey) {
          Serial.println("Remember to type 'save' to persist changes!");
        }
      } else {
        Serial.println("✗ Usage: set <key> <value>");
        Serial.println("Example: set wifi_ssid MyNetwork");
      }
    }
    else if (cmd == "nosleep" || cmd == "stay" || cmd == "awake") {
      config.deepSleepEnabled = false;
      config.saveConfig();
      Serial.println("✓ Deep sleep disabled and saved");
      Serial.println("Device will stay awake for debugging");
    }
    else if (cmd == "sleep") {
      config.deepSleepEnabled = true;
      config.saveConfig();
      Serial.println("✓ Deep sleep enabled and saved");
      Serial.println("Device will enter deep sleep after next reading");
    }
    else if (cmd == "reboot" || cmd == "restart") {
      Serial.println("Rebooting...");
      delay(500);
      ESP.restart();
    }
    else if (cmd == "help") {
      Serial.println("\n╔═══════════════════════════════════════════════════════╗");
      Serial.println("║   Battery Monitor - Serial Commands                   ║");
      Serial.println("╚═══════════════════════════════════════════════════════╝");
      Serial.println("\nConfiguration Commands:");
      Serial.println("  show              - Display current configuration");
      Serial.println("  set <key> <value> - Change a configuration value");
      Serial.println("  save              - Save configuration to NVS");
      Serial.println("  reset nvs         - Clear NVS and reboot");
      Serial.println("\nConfiguration Keys:");
      Serial.println("  wifi_ssid         - WiFi network name");
      Serial.println("  wifi_password     - WiFi password");
      Serial.println("  mqtt_server       - MQTT broker address");
      Serial.println("  mqtt_port         - MQTT broker port");
      Serial.println("  mqtt_user         - MQTT username");
      Serial.println("  mqtt_password     - MQTT password");
      Serial.println("  mqtt_client_id    - MQTT client identifier");
      Serial.println("  deep_sleep        - Enable/disable deep sleep (true/false)");
      Serial.println("\nSystem Commands:");
      Serial.println("  nosleep           - Disable deep sleep (stay awake)");
      Serial.println("  sleep             - Enable deep sleep");
      Serial.println("  reboot            - Restart the device");
      Serial.println("  help              - Show this help");
      Serial.println("\nExample Usage:");
      Serial.println("  > set wifi_ssid MyNetwork");
      Serial.println("  > set wifi_password MyPassword123");
      Serial.println("  > set mqtt_server mqtt.home.local");
      Serial.println("  > save");
      Serial.println("  > reboot\n");
    }
    else if (command.length() > 0) {
      Serial.print("✗ Unknown command: ");
      Serial.println(command);
      Serial.println("Type 'help' for available commands");
    }
  }
}

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

void setupOTA() {
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

void handleOTAMode() {
  Serial.println("\n╔═══════════════════════════════╗");
  Serial.println("║   Entering OTA Mode           ║");
  Serial.println("╚═══════════════════════════════╝");  
  // Check if filename is provided for HTTP update
  if (otaFilename.length() > 0) {
    // HTTP OTA - Download from base URL + filename
    Serial.println("Mode: HTTP Update from Base URL");
    
    if (performHTTPUpdate(otaFilename)) {
      Serial.println("Update successful! Device will reboot...");
      delay(1000);
      ESP.restart();
    } else {
      Serial.println("Update failed. Continuing normal operation.");
    }
    
    otaRequested = false;
    otaFilename = "";
    return;
  }
  
  // ArduinoOTA - Wait for direct network upload
  Serial.println("Mode: ArduinoOTA (Network Upload)");  Serial.println("Waiting for OTA update...");
  Serial.println("Device will stay awake for 5 minutes");
  Serial.print("Hostname: ");
  Serial.println(config.mqttClientID);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  setupOTA();
  
  // Stay awake for 5 minutes to allow OTA update
  unsigned long otaStartTime = millis();
  unsigned long otaTimeout = 5 * 60 * 1000;  // 5 minutes
  
  while (millis() - otaStartTime < otaTimeout) {
    ArduinoOTA.handle();
    mqttClient.loop();  // Keep MQTT connection alive
    delay(10);
    
    // Print countdown every 30 seconds
    if ((millis() - otaStartTime) % 30000 < 50) {
      unsigned long remaining = (otaTimeout - (millis() - otaStartTime)) / 1000;
      Serial.print("Time remaining: ");
      Serial.print(remaining);
      Serial.println(" seconds");
    }
  }
  
  Serial.println("\nOTA timeout reached. Resuming normal operation.");
  otaRequested = false;
}

void enterDeepSleep() {
  Serial.println("\n─────────────────────────────────");
  Serial.println("Entering deep sleep mode...");
  Serial.print("Next reading in: ");
  Serial.print(Config::DEEP_SLEEP_INTERVAL_US / 1000000);
  Serial.println(" seconds");
  Serial.println("Power consumption: ~10 µA");
  Serial.println("─────────────────────────────────");
  Serial.flush();  // Wait for serial transmission to complete
  
  // Configure timer wakeup
  esp_sleep_enable_timer_wakeup(Config::DEEP_SLEEP_INTERVAL_US);
  
  // Enter deep sleep
  esp_deep_sleep_start();
}

void setup() {
  // Initialize serial communication
  Serial.begin(Config::SERIAL_BAUD_RATE);
  delay(500);  // Short delay for serial connection
  
  // Increment boot count
  bootCount++;
  
  // Print boot information
  Serial.println("\n╔═════════════════════════════════════╗");
  Serial.println("║  ESP32 Battery Monitor (Deep Sleep) ║");
  Serial.println("╚═════════════════════════════════════╝");
  Serial.print("Boot count: ");
  Serial.println(bootCount);
  
  // Print wakeup reason
  printWakeupReason();
  
  if (bootCount > 1) {
    Serial.print("Last voltage: ");
    Serial.print(lastVoltage, 2);
    Serial.println(" V");
  }
  
  Serial.println();
  
  // Initialize configuration manager (loads from NVS or uses defaults on first run)
  config.begin(WIFI_SSID, WIFI_PASSWORD, 
               MQTT_SERVER, MQTT_PORT, 
               MQTT_USER, MQTT_PASSWORD, 
               MQTT_CLIENT_ID);
  
  // Initialize battery monitor
  monitor.begin();
  
  // Print battery type info (only on first boot)
  if (bootCount == 1) {
    monitor.printStartupInfo();
  }
}

void loop() {
  // Take reading immediately
  BatteryReading reading = monitor.readBattery();
  
  // Store voltage in RTC memory
  lastVoltage = reading.voltage;
  
  // Display reading
  monitor.printReading(reading);
  
  // Connect to WiFi and MQTT, then publish
  Serial.println("\n─────────────────────────────────");
  wifiConnected = connectWiFi();
  
  if (wifiConnected) {
    mqttConnected = connectMQTT();
    
    if (mqttConnected) {
      publishToMQTT(reading);
      
      // Process MQTT messages for a few seconds to check for OTA trigger
      Serial.println("Checking for MQTT commands...");
      unsigned long checkStart = millis();
      while (millis() - checkStart < 3000) {
        mqttClient.loop();
        delay(100);
        
        // If OTA is requested, enter OTA mode
        if (otaRequested) {
          handleOTAMode();
          break;
        }
      }
    }
    
    // Disconnect to save power (unless in OTA mode)
    if (!otaRequested) {
      mqttClient.disconnect();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
  }
  Serial.println("─────────────────────────────────");
  
  // Check for serial commands (for configuration/debugging)
  checkSerialCommands();
  
  // Additional info
  Serial.print("Time awake: ");
  Serial.print(millis());
  Serial.println(" ms");
  
  if (config.deepSleepEnabled && Config::ENABLE_DEEP_SLEEP) {
    // On first boot, wait longer to allow serial commands
    if (bootCount == 1) {
      Serial.println("First boot: waiting 30 seconds before deep sleep...");
      Serial.println("Type 'nosleep' to keep device awake.");
      unsigned long waitStart = millis();
      while (millis() - waitStart < 30000) {
        checkSerialCommands();
        delay(200);
      }
    }

    // Wait a moment for any processing
    delay(2000);
    
    // Enter deep sleep
    enterDeepSleep();
  } else {
    // Fall back to normal delay if deep sleep disabled
    Serial.println("Deep sleep disabled, waiting...");
    Serial.println("Type 'sleep' to re-enable deep sleep");
    delay(Config::READING_INTERVAL_MS);
  }
}
