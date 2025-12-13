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
#include "battery_monitor.h"
#include "esp_sleep.h"

// Include credentials (create these files!)
#include "wifi_credentials.h"  // Defines WIFI_SSID and WIFI_PASSWORD
#include "mqtt_credentials.h"  // Defines MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_CLIENT_ID

// RTC memory to preserve data across deep sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR float lastVoltage = 0.0;

// Global objects
BatteryMonitor monitor;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Connection status flags
bool wifiConnected = false;
bool mqttConnected = false;

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
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
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
    return false;
  }
}

bool connectMQTT() {
  Serial.print("Connecting to MQTT broker");
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  
  unsigned long startTime = millis();
  while (!mqttClient.connected() && millis() - startTime < Config::MQTT_TIMEOUT_MS) {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println(" Connected!");
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
  Serial.println("\n╔═════════════════════════════════╗");
  Serial.println("║  ESP32 Battery Monitor (Deep Sleep) ║");
  Serial.println("╚═════════════════════════════════╝");
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
      mqttClient.loop();  // Process any pending messages
      delay(100);  // Brief delay to ensure publish completes
    }
    
    // Disconnect to save power
    mqttClient.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
  Serial.println("─────────────────────────────────");
  
  // Additional info
  Serial.print("Time awake: ");
  Serial.print(millis());
  Serial.println(" ms");
  
  if (Config::ENABLE_DEEP_SLEEP) {
    // Wait a moment for any processing
    delay(2000);
    
    // Enter deep sleep
    enterDeepSleep();
  } else {
    // Fall back to normal delay if deep sleep disabled
    Serial.println("Deep sleep disabled, waiting...");
    delay(Config::READING_INTERVAL_MS);
  }
}
