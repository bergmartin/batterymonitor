/*
 * ESP32 Battery Voltage Monitor
 * 
 * Monitors a 12V battery using a voltage divider circuit
 * Supports Lead-Acid and LiFePO4 battery types
 * 
 * Hardware Setup:
 * - Battery + terminal -> R1 (30kΩ) -> ADC Pin (GPIO34) -> R2 (10kΩ) -> GND
 * - This creates a 4:1 voltage divider (12V becomes ~3V at ADC)
 * 
 * ADC Resolution: 12-bit (0-4095)
 * ADC Reference Voltage: 3.3V
 * Voltage Divider Ratio: 4.0 (R1=30kΩ, R2=10kΩ)
 * 
 * Battery Type: Set in platformio.ini with -D BATTERY_TYPE=LEAD_ACID or LIFEPO4
 */

#include <Arduino.h>
#include "battery_monitor.h"

// Global objects
BatteryMonitor monitor;

// Timing
unsigned long lastReadingTime = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(Config::SERIAL_BAUD_RATE);
  delay(Config::STARTUP_DELAY_MS);
  
  // Initialize battery monitor
  monitor.begin();
  
  // Print startup information
  monitor.printStartupInfo();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check if it's time for a new reading
  if (currentTime - lastReadingTime >= Config::READING_INTERVAL_MS) {
    lastReadingTime = currentTime;
    
    // Read battery and display information
    BatteryReading reading = monitor.readBattery();
    monitor.printReading(reading);
  }
}
