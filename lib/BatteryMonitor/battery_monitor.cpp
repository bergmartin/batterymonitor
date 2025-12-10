/*
 * Battery Monitor Library Implementation
 */

#include "battery_monitor.h"

// ============================================================================
// Legacy Constants (for backward compatibility with tests)
// ============================================================================
const int BATTERY_PIN = Config::BATTERY_ADC_PIN;
const float VOLTAGE_DIVIDER_RATIO = Config::VOLTAGE_DIVIDER_RATIO;
const float ADC_REFERENCE_VOLTAGE = Config::ADC_REFERENCE_VOLTAGE;
const int ADC_RESOLUTION = Config::ADC_MAX_VALUE;
const char* BATTERY_TYPE_NAME = Config::BATTERY_TYPE_NAME;
const float VOLTAGE_FULL = Config::Voltage::FULL;
const float VOLTAGE_NOMINAL = Config::Voltage::NOMINAL;
const float VOLTAGE_LOW = Config::Voltage::LOW_THRESHOLD;
const float VOLTAGE_CRITICAL = Config::Voltage::CRITICAL;
const float VOLTAGE_MIN = Config::Voltage::MINIMUM;

// ============================================================================
// BatteryMonitor Class Implementation
// ============================================================================

BatteryMonitor::BatteryMonitor() {
  // Constructor
}

void BatteryMonitor::begin() {
  // Configure ADC
  analogReadResolution(Config::ADC_RESOLUTION_BITS);
  analogSetAttenuation(ADC_11db);  // 0-3.3V range
}

int BatteryMonitor::readADC() {
  long sum = 0;
  
  // Take multiple samples and average for better accuracy
  for (int i = 0; i < Config::SAMPLE_COUNT; i++) {
    sum += analogRead(Config::BATTERY_ADC_PIN);
    delay(Config::SAMPLE_DELAY_MS);
  }
  
  return sum / Config::SAMPLE_COUNT;
}

float BatteryMonitor::adcToVoltage(int adcValue) {
  // Convert ADC reading to voltage at ADC pin
  float adcVoltage = (adcValue / (float)Config::ADC_MAX_VALUE) * Config::ADC_REFERENCE_VOLTAGE;
  
  // Calculate actual battery voltage using voltage divider ratio
  return adcVoltage * Config::VOLTAGE_DIVIDER_RATIO;
}

float BatteryMonitor::readVoltage() {
  int adcValue = readADC();
  return adcToVoltage(adcValue);
}

BatteryReading BatteryMonitor::readBattery() {
  BatteryReading reading;
  reading.voltage = readVoltage();
  reading.percentage = calculatePercentage(reading.voltage);
  reading.status = determineStatus(reading.voltage);
  reading.timestamp = millis();
  return reading;
}

float BatteryMonitor::calculatePercentage(float voltage) {
  // Clamp to 100% if above full voltage
  if (voltage >= Config::Voltage::FULL) {
    return 100.0f;
  }
  
  // Clamp to 0% if below minimum voltage
  if (voltage <= Config::Voltage::MINIMUM) {
    return 0.0f;
  }
  
  // Linear interpolation between minimum and full
  float range = Config::Voltage::FULL - Config::Voltage::MINIMUM;
  float percentage = ((voltage - Config::Voltage::MINIMUM) / range) * 100.0f;
  
  return constrain(percentage, 0.0f, 100.0f);
}

BatteryStatus BatteryMonitor::determineStatus(float voltage) {
  if (voltage >= Config::Voltage::FULL) {
    return BatteryStatus::FULL;
  } else if (voltage >= Config::Voltage::NOMINAL) {
    return BatteryStatus::GOOD;
  } else if (voltage >= Config::Voltage::LOW_THRESHOLD) {
    return BatteryStatus::LOW_BATTERY;
  } else if (voltage >= Config::Voltage::CRITICAL) {
    return BatteryStatus::CRITICAL;
  } else {
    return BatteryStatus::DEAD;
  }
}

const char* BatteryMonitor::statusToString(BatteryStatus status) {
  switch (status) {
    case BatteryStatus::FULL:        return "FULL";
    case BatteryStatus::GOOD:        return "GOOD";
    case BatteryStatus::LOW_BATTERY: return "LOW";
    case BatteryStatus::CRITICAL:    return "CRITICAL";
    case BatteryStatus::DEAD:        return "DEAD";
    default:                         return "UNKNOWN";
  }
}

void BatteryMonitor::printStartupInfo() {
  Serial.println("\n=================================");
  Serial.println("ESP32 Battery Voltage Monitor");
  Serial.println("=================================");
  Serial.print("Battery Type: ");
  Serial.println(Config::BATTERY_TYPE_NAME);
  Serial.print("Voltage Range: ");
  Serial.print(Config::Voltage::MINIMUM, 1);
  Serial.print("V - ");
  Serial.print(Config::Voltage::FULL, 1);
  Serial.println("V");
  Serial.println("=================================\n");
}

void BatteryMonitor::printReading(const BatteryReading& reading) {
  Serial.println("─────────────────────────────────");
  
  // Voltage
  Serial.print("Battery Voltage: ");
  Serial.print(reading.voltage, 2);
  Serial.println(" V");
  
  // Percentage
  Serial.print("Battery Level:   ");
  Serial.print(reading.percentage, 1);
  Serial.println(" %");
  
  // Status
  Serial.print("Status:          ");
  Serial.println(statusToString(reading.status));
  
  // Visual indicator
  Serial.print("Battery: [");
  int bars = (int)(reading.percentage / 10.0f);
  for (int i = 0; i < 10; i++) {
    Serial.print(i < bars ? "█" : "░");
  }
  Serial.println("]");
  
  // Warnings
  if (reading.status == BatteryStatus::LOW_BATTERY) {
    Serial.println("⚠️  WARNING: Low battery!");
  }
  if (reading.status == BatteryStatus::CRITICAL) {
    Serial.println("🔴 CRITICAL: Battery needs immediate charging!");
  }
  if (reading.status == BatteryStatus::DEAD) {
    Serial.println("💀 DEAD: Battery voltage too low!");
  }
  
  Serial.println();
}

// ============================================================================
// Legacy Function Compatibility (for existing tests)
// ============================================================================

float calculateBatteryPercentage(float voltage) {
  return BatteryMonitor::calculatePercentage(voltage);
}

String getBatteryStatus(float voltage) {
  BatteryStatus status = BatteryMonitor::determineStatus(voltage);
  return String(BatteryMonitor::statusToString(status));
}

float adcToBatteryVoltage(int adcReading) {
  return BatteryMonitor::adcToVoltage(adcReading);
}
