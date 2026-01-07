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
#include "config_manager.h"
#include "network_manager.h"
#include "ota_manager.h"
#include "command_handler.h"
#include "display_manager.h"

// Include credentials (create these files!)
// These are now used as DEFAULT VALUES only - actual credentials stored in NVS
#include "wifi_credentials.h" // Defines WIFI_SSID and WIFI_PASSWORD
#include "mqtt_credentials.h" // Defines MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD, MQTT_CLIENT_ID

// RTC memory to preserve data across deep sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR float lastVoltage = 0.0;

// Global objects
BatteryMonitor monitor;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
ConfigManager config; // Manages credentials in NVS (persists across OTA updates)
NetworkManager network(wifiClient, mqttClient, config);
CommandHandler commandHandler(config);
DisplayManager display;
OTAManager otaManager(config, &display);

void printWakeupReason()
{
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
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

void enterDeepSleep()
{
  // Calculate next wakeup time
  time_t now;
  time(&now);
  time_t wakeupTime = now + (Config::DEEP_SLEEP_INTERVAL_US / 1000000);
  struct tm* timeinfo = localtime(&wakeupTime);
  
  char wakeupISO[25];
  strftime(wakeupISO, sizeof(wakeupISO), "%Y-%m-%dT%H:%M:%S", timeinfo);
  
  Serial.println("\n─────────────────────────────────");
  Serial.println("Entering deep sleep mode...");
  Serial.print("Next reading in: ");
  Serial.print(Config::DEEP_SLEEP_INTERVAL_US / 1000000);
  Serial.println(" seconds");
  Serial.print("Wake at: ");
  Serial.println(wakeupISO);
  Serial.println("Power consumption: ~10 µA");
  Serial.println("─────────────────────────────────");
  
  // Show sleep screen on display
  if (display.isReady()) {
    display.showSleepScreen(wakeupISO);
    delay(2000); // Show sleep screen for 2 seconds
  }
  
  Serial.flush(); // Wait for serial transmission to complete

  // Configure timer wakeup
  esp_sleep_enable_timer_wakeup(Config::DEEP_SLEEP_INTERVAL_US);

  // Enter deep sleep
  esp_deep_sleep_start();
}

void setup()
{
  // Initialize serial communication
  Serial.begin(Config::SERIAL_BAUD_RATE);
  delay(500); // Short delay for serial connection

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

  if (bootCount > 1)
  {
    Serial.print("Last voltage: ");
    Serial.print(lastVoltage, 2);
    Serial.println(" V");
  }

  Serial.println();

  // Initialize display early
  display.begin();
  if (display.isReady()) {
    display.showBootScreen(bootCount);
    delay(1000); // Show boot screen for 1 second
  }

  // Initialize configuration manager (loads from NVS or uses defaults on first run)
  config.begin(WIFI_SSID, WIFI_PASSWORD,
               MQTT_SERVER, MQTT_PORT,
               MQTT_USER, MQTT_PASSWORD,
               MQTT_CLIENT_ID);

  // Apply battery chemistry from NVS
  if (config.batteryType.equalsIgnoreCase("lifepo4")) {
    BatteryMonitor::setChemistry(BatteryChemistry::LIFEPO4);
    Serial.println("Battery chemistry set from NVS: LiFePO4");
  } else {
    BatteryMonitor::setChemistry(BatteryChemistry::LEAD_ACID);
    Serial.println("Battery chemistry set from NVS: Lead-Acid");
  }

  // Check for pending OTA update from previous wake cycle
  if (otaManager.checkPendingOTA())
  {
    Serial.println("OTA update was triggered while device was asleep.");
    Serial.println("Processing OTA update now...");
    
    if (display.isReady()) {
      display.showOTAScreen("Processing...");
    }
    
    // Connect to WiFi for OTA
    if (network.connectWiFi())
    {
      otaManager.setup();
      otaManager.handleUpdate();
      // If we reach here, OTA timed out or failed
      // Continue with normal operation
      network.disconnect();
    }
    else
    {
      Serial.println("Failed to connect to WiFi for OTA. Will retry next boot.");
    }
  }if (display.isReady()) {
      display.showOTAScreen("Checking...");
    }
    
    
  // Automatically check for available updates on every wake
  else if (Config::AUTO_CHECK_OTA)
  {
    // Connect to WiFi to check for updates
    if (network.connectWiFi())
    {
      // Check if a newer version is available
      if (otaManager.checkForUpdates())
      {
        // New version found and OTA requested
        otaManager.setup();
        otaManager.handleUpdate();
        // If we reach here, OTA timed out or failed
        // Continue with normal operation
      }
      network.disconnect();
    }
    else
    {
      Serial.println("Failed to connect to WiFi for update check.");
    }
  }

  // Initialize battery monitor
  monitor.begin();

  // Print battery type info (only on first boot)
  if (bootCount == 1)
  {
    monitor.printStartupInfo();
  }

  // Set up OTA and network callbacks
  network.setOTACallback([](const String &filename)
                         { otaManager.requestUpdate(filename); });

  network.setResetCallback([]()
                           {
    config.clear();
    Serial.println("NVS will be cleared. Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart(); });
}

void loop()
{
  // Take reading immediately
  BatteryReading reading = monitor.readBattery();

  // Store voltage in RTC memory
  lastVoltage = reading.voltage;

  // Display reading
  monitor.printReading(reading);

  // Update display with battery info (WiFi not connected yet)
  if (display.isReady()) {
    display.update(reading, false, 0);
  }

  // Connect to WiFi and MQTT, then publish
  Serial.println("\n─────────────────────────────────");
  if (network.connectWiFi())
  {
    // Get WiFi RSSI
    int8_t rssi = WiFi.RSSI();
    
    // Update display with WiFi info
    if (display.isReady()) {
      display.update(reading, true, rssi);
    }
    // Initialize OTA early (only once per wake cycle)
    static bool otaInitialized = false;
    if (!otaInitialized)
    {
      otaManager.setup();
      otaInitialized = true;
    }

    if (network.connectMQTT())
    {
      network.publishReading(reading, bootCount);

      // Process MQTT messages for a few seconds to check for OTA trigger
      Serial.println("Checking for MQTT commands...");
      unsigned long checkStart = millis();
      while (millis() - checkStart < 3000)
      {
        network.loop();
        
        // Update display periodically
        if (display.isReady() && millis() % 500 < 100) {
          display.update(reading, true, WiFi.RSSI());
        }
        
        delay(100);

        // If OTA is requested, handle it
        if (otaManager.isUpdateRequested())
        {
          if (display.isReady()) {
            display.showOTAScreen("Starting...");
          }
          otaManager.handleUpdate();
          break;
        }
      }
    }
    // ...
    // Disconnect to save power (unless in OTA mode)
    if (!otaManager.isUpdateRequested())
    {
      network.disconnect();
    }
  }
  Serial.println("─────────────────────────────────");

  // Check for serial commands (for configuration/debugging)
  commandHandler.checkCommands();

  // Additional info
  Serial.print("Time awake: ");
  Serial.print(millis());
  Serial.println(" ms");

  // Don't enter deep sleep if OTA update is requested/in progress
  if (otaManager.isUpdateRequested())
  {
    Serial.println("OTA update requested - staying awake");
    Serial.println("Device will remain active to handle OTA update");
    delay(Config::READING_INTERVAL_MS);
  }
  else if (config.deepSleepEnabled && Config::ENABLE_DEEP_SLEEP)
  {
    // On first boot, wait longer to allow serial commands
    if (bootCount == 1)
    {
      Serial.println("First boot: waiting 30 seconds before deep sleep...");
      Serial.println("Type 'nosleep' to keep device awake.");
      unsigned long waitStart = millis();
      while (millis() - waitStart < 30000)
      {
        commandHandler.checkCommands();
        delay(200);
      }
    }

    // Wait a moment for any processing
    delay(2000);

    // Enter deep sleep
    enterDeepSleep();
  }
  else
  {
    // Fall back to normal delay if deep sleep disabled
    Serial.println("Deep sleep disabled, waiting...");
    Serial.println("Type 'sleep' to re-enable deep sleep");
    delay(Config::READING_INTERVAL_MS);
  }
}
