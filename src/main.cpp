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

// Battery type definitions
#define LEAD_ACID 1
#define LIFEPO4 2

// Default to Lead-Acid if not specified
#ifndef BATTERY_TYPE
#define BATTERY_TYPE LEAD_ACID
#endif

// Configuration
const int BATTERY_PIN = 34;           // ADC1_CH6 (GPIO34) - ADC pin for voltage reading
const float VOLTAGE_DIVIDER_RATIO = 4.0;  // (R1 + R2) / R2 = (30k + 10k) / 10k
const float ADC_REFERENCE_VOLTAGE = 3.3;  // ESP32 ADC reference voltage
const int ADC_RESOLUTION = 4095;      // 12-bit ADC (0-4095)
const int SAMPLES = 10;               // Number of samples for averaging
const unsigned long READING_INTERVAL = 2000;  // Read every 2 seconds

// Battery voltage thresholds - configured based on battery type
#if BATTERY_TYPE == LEAD_ACID
  const char* BATTERY_TYPE_NAME = "Lead-Acid";
  const float VOLTAGE_FULL = 12.7;      // Fully charged
  const float VOLTAGE_NOMINAL = 12.4;   // 75% charged
  const float VOLTAGE_LOW = 12.0;       // 25% - should recharge soon
  const float VOLTAGE_CRITICAL = 11.8;  // Discharged - recharge now
  const float VOLTAGE_MIN = 10.5;       // Minimum safe voltage
#elif BATTERY_TYPE == LIFEPO4
  const char* BATTERY_TYPE_NAME = "LiFePO4";
  const float VOLTAGE_FULL = 14.6;      // Fully charged (4S = 4 × 3.65V)
  const float VOLTAGE_NOMINAL = 13.2;   // 75% charged
  const float VOLTAGE_LOW = 12.8;       // 25% - should recharge soon
  const float VOLTAGE_CRITICAL = 12.0;  // Discharged - recharge now
  const float VOLTAGE_MIN = 10.0;       // Minimum safe voltage (4S = 4 × 2.5V)
#else
  #error "Invalid BATTERY_TYPE. Use LEAD_ACID or LIFEPO4"
#endif

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
  
  // Convert ADC reading to voltage
  float adcVoltage = (average / ADC_RESOLUTION) * ADC_REFERENCE_VOLTAGE;
  
  // Calculate actual battery voltage using voltage divider ratio
  float batteryVoltage = adcVoltage * VOLTAGE_DIVIDER_RATIO;
  
  return batteryVoltage;
}

float calculateBatteryPercentage(float voltage) {
  // Simple linear mapping based on battery discharge curve
  if (voltage >= VOLTAGE_FULL) return 100.0;
  if (voltage <= VOLTAGE_MIN) return 0.0;
  
  float percentage = ((voltage - VOLTAGE_MIN) / (VOLTAGE_FULL - VOLTAGE_MIN)) * 100.0;
  return constrain(percentage, 0.0, 100.0);
}

String getBatteryStatus(float voltage) {
  if (voltage >= VOLTAGE_FULL) {
    return "FULL";
  } else if (voltage >= VOLTAGE_NOMINAL) {
    return "GOOD";
  } else if (voltage >= VOLTAGE_LOW) {
    return "LOW";
  } else if (voltage >= VOLTAGE_CRITICAL) {
    return "CRITICAL";
  } else {
    return "DEAD";
  }
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
