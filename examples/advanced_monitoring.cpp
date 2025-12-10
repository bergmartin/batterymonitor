/*
 * Advanced Example - Custom Battery Monitoring
 * 
 * This example demonstrates advanced usage of the BatteryMonitor library:
 * - Using the BatteryReading structure
 * - Implementing custom alerts
 * - Data logging
 * - Custom thresholds
 */

#include <Arduino.h>
#include "battery_monitor.h"

// Create battery monitor instance
BatteryMonitor monitor;

// Data logging
struct BatteryLog {
  float minVoltage = 999.0;
  float maxVoltage = 0.0;
  float avgVoltage = 0.0;
  int readingCount = 0;
  unsigned long sessionStart = 0;
};

BatteryLog log;

// Custom alert thresholds
const float CUSTOM_ALERT_VOLTAGE = 11.5;  // Custom alert threshold
bool alertSent = false;

void logReading(const BatteryReading& reading) {
  // Update statistics
  if (reading.voltage < log.minVoltage) {
    log.minVoltage = reading.voltage;
  }
  if (reading.voltage > log.maxVoltage) {
    log.maxVoltage = reading.voltage;
  }
  
  // Update running average
  log.avgVoltage = ((log.avgVoltage * log.readingCount) + reading.voltage) / (log.readingCount + 1);
  log.readingCount++;
}

void printStatistics() {
  unsigned long uptime = (millis() - log.sessionStart) / 1000;
  
  Serial.println("\n╔════════════════════════════════╗");
  Serial.println("║     Session Statistics         ║");
  Serial.println("╠════════════════════════════════╣");
  Serial.print("║ Uptime:     ");
  Serial.print(uptime);
  Serial.println(" seconds");
  Serial.print("║ Readings:   ");
  Serial.println(log.readingCount);
  Serial.print("║ Min Volt:   ");
  Serial.print(log.minVoltage, 2);
  Serial.println(" V");
  Serial.print("║ Max Volt:   ");
  Serial.print(log.maxVoltage, 2);
  Serial.println(" V");
  Serial.print("║ Avg Volt:   ");
  Serial.print(log.avgVoltage, 2);
  Serial.println(" V");
  Serial.println("╚════════════════════════════════╝\n");
}

void checkCustomAlerts(const BatteryReading& reading) {
  // Example: Send alert when voltage drops below custom threshold
  if (reading.voltage < CUSTOM_ALERT_VOLTAGE && !alertSent) {
    Serial.println("\n🚨 CUSTOM ALERT: Voltage below " + String(CUSTOM_ALERT_VOLTAGE, 1) + "V!");
    Serial.println("Consider connecting to charger.\n");
    alertSent = true;
  }
  
  // Reset alert when voltage recovers
  if (reading.voltage >= CUSTOM_ALERT_VOLTAGE + 0.2 && alertSent) {
    alertSent = false;
  }
}

void setup() {
  Serial.begin(Config::SERIAL_BAUD_RATE);
  delay(Config::STARTUP_DELAY_MS);
  
  // Initialize monitor
  monitor.begin();
  monitor.printStartupInfo();
  
  // Initialize logging
  log.sessionStart = millis();
  
  Serial.println("Advanced Battery Monitoring Started");
  Serial.println("Custom alert threshold: " + String(CUSTOM_ALERT_VOLTAGE, 1) + "V\n");
}

void loop() {
  static unsigned long lastReading = 0;
  static unsigned long lastStats = 0;
  
  unsigned long currentTime = millis();
  
  // Take reading every 2 seconds
  if (currentTime - lastReading >= 2000) {
    lastReading = currentTime;
    
    // Read battery
    BatteryReading reading = monitor.readBattery();
    
    // Display reading
    monitor.printReading(reading);
    
    // Log data
    logReading(reading);
    
    // Check custom alerts
    checkCustomAlerts(reading);
  }
  
  // Print statistics every 30 seconds
  if (currentTime - lastStats >= 30000) {
    lastStats = currentTime;
    printStatistics();
  }
}
