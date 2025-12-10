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

// Configuration
const int SAMPLES = 10;               // Number of samples for averaging
const unsigned long READING_INTERVAL = 2000;  // Read every 2 seconds

// Variables
unsigned long lastReadingTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Configure ADC
  analogReadResolution(12);  // Set 12-bit resolution
  analogSetAttenuation(ADC_11db);  // Set attenuation for 0-3.3V range
  
  delay(1000);
  Serial.println("\n=================================");
  Serial.println("ESP32 Battery Voltage Monitor");
  Serial.println("=================================");
  Serial.print("Battery Type: ");
  Serial.println(BATTERY_TYPE_NAME);
  Serial.print("Voltage Range: ");
  Serial.print(VOLTAGE_MIN, 1);
  Serial.print("V - ");
  Serial.print(VOLTAGE_FULL, 1);
  Serial.println("V");
  Serial.println();
}

float readBatteryVoltage() {
  long sum = 0;
  
  // Take multiple samples and average them for accuracy
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(BATTERY_PIN);
    delay(10);
  }
  
  float average = sum / (float)SAMPLES;
  
  // Convert ADC reading to battery voltage
  return adcToBatteryVoltage((int)average);
}

void displayBatteryInfo(float voltage) {
  float percentage = calculateBatteryPercentage(voltage);
  String status = getBatteryStatus(voltage);
  
  Serial.println("─────────────────────────────────");
  Serial.print("Battery Voltage: ");
  Serial.print(voltage, 2);
  Serial.println(" V");
  
  Serial.print("Battery Level:   ");
  Serial.print(percentage, 1);
  Serial.println(" %");
  
  Serial.print("Status:          ");
  Serial.println(status);
  
  // Visual battery indicator
  Serial.print("Battery: [");
  int bars = (int)(percentage / 10);
  for (int i = 0; i < 10; i++) {
    if (i < bars) {
      Serial.print("█");
    } else {
      Serial.print("░");
    }
  }
  Serial.println("]");
  
  // Warnings
  if (voltage < VOLTAGE_LOW) {
    Serial.println("⚠️  WARNING: Low battery!");
  }
  if (voltage < VOLTAGE_CRITICAL) {
    Serial.println("🔴 CRITICAL: Battery needs immediate charging!");
  }
  
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read battery voltage at specified interval
  if (currentTime - lastReadingTime >= READING_INTERVAL) {
    lastReadingTime = currentTime;
    
    float voltage = readBatteryVoltage();
    displayBatteryInfo(voltage);
  }
}
