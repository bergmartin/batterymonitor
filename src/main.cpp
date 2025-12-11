/*
 * ESP32 Battery Voltage Monitor with Deep Sleep
 * 
 * Monitors a 12V battery using a voltage divider circuit
 * Supports Lead-Acid and LiFePO4 battery types
 * Features deep sleep mode for ultra-low power consumption
 * 
 * Hardware Setup:
 * - Battery + terminal -> R1 (30kΩ) -> ADC Pin (GPIO34) -> R2 (10kΩ) -> GND
 * - This creates a 4:1 voltage divider (12V becomes ~3V at ADC)
 * 
 * Power Consumption:
 * - Active: ~80-160 mA (during reading, ~5 seconds)
 * - Deep Sleep: ~0.01 mA (between readings, 1 hour)
 * - Average: ~0.1-0.3 mA (>99.9% time in deep sleep)
 * 
 * Battery Type: Set in platformio.ini with -D BATTERY_TYPE=LEAD_ACID or LIFEPO4
 */

#include <Arduino.h>
#include "battery_monitor.h"
#include "esp_sleep.h"

// RTC memory to preserve data across deep sleep
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR float lastVoltage = 0.0;

// Global objects
BatteryMonitor monitor;

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
