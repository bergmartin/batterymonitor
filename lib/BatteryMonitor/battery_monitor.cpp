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
// Dynamic values based on runtime chemistry
const char* BATTERY_TYPE_NAME = "Lead-Acid";
float VOLTAGE_FULL = 12.7f;
float VOLTAGE_NOMINAL = 12.4f;
float VOLTAGE_LOW = 12.0f;
float VOLTAGE_CRITICAL = 11.8f;
float VOLTAGE_MIN = 10.5f;

// Internal chemistry state
static BatteryChemistry activeChemistry = BatteryChemistry::LEAD_ACID;

// Threshold sets
struct Thresholds {
  const char* name;
  float FULL;
  float NOMINAL;
  float LOW_THRESHOLD;
  float CRITICAL;
  float MINIMUM;
};

static constexpr Thresholds LEAD_ACID_THRESHOLDS{
  "Lead-Acid",
  12.7f,
  12.4f,
  12.0f,
  11.8f,
  10.5f
};

static constexpr Thresholds LIFEPO4_THRESHOLDS{
  "LiFePO4",
  14.6f,
  13.2f,
  12.8f,
  12.0f,
  10.0f
};

static const Thresholds& getThresholds() {
  return (activeChemistry == BatteryChemistry::LIFEPO4) ? LIFEPO4_THRESHOLDS : LEAD_ACID_THRESHOLDS;
}

void BatteryMonitor::setChemistry(BatteryChemistry chemistry) {
  activeChemistry = chemistry;
  const Thresholds& t = getThresholds();
  BATTERY_TYPE_NAME = t.name;
  VOLTAGE_FULL = t.FULL;
  VOLTAGE_NOMINAL = t.NOMINAL;
  VOLTAGE_LOW = t.LOW_THRESHOLD;
  VOLTAGE_CRITICAL = t.CRITICAL;
  VOLTAGE_MIN = t.MINIMUM;
}

BatteryChemistry BatteryMonitor::getChemistry() {
  return activeChemistry;
}

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
  if (voltage >= VOLTAGE_FULL) {
    return 100.0f;
  }
  
  // Clamp to 0% if below minimum voltage
  if (voltage <= VOLTAGE_MIN) {
    return 0.0f;
  }
  
  // Linear interpolation between minimum and full
  float range = VOLTAGE_FULL - VOLTAGE_MIN;
  float percentage = ((voltage - VOLTAGE_MIN) / range) * 100.0f;
  
  return constrain(percentage, 0.0f, 100.0f);
}

BatteryStatus BatteryMonitor::determineStatus(float voltage) {
  if (voltage >= VOLTAGE_FULL) {
    return BatteryStatus::FULL;
  } else if (voltage >= VOLTAGE_NOMINAL) {
    return BatteryStatus::GOOD;
  } else if (voltage >= VOLTAGE_LOW) {
    return BatteryStatus::LOW_BATTERY;
  } else if (voltage >= VOLTAGE_CRITICAL) {
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
  Serial.println(getBatteryTypeName());
  Serial.print("Voltage Range: ");
  Serial.print(VOLTAGE_MIN, 1);
  Serial.print("V - ");
  Serial.print(VOLTAGE_FULL, 1);
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

// Runtime getters
const char* BatteryMonitor::getBatteryTypeName() {
  return getThresholds().name;
}

float BatteryMonitor::getMinVoltage() {
  return getThresholds().MINIMUM;
}

float BatteryMonitor::getMaxVoltage() {
  return getThresholds().FULL;
}
