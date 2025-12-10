/*
 * Battery Monitor Library Implementation
 */

#include "battery_monitor.h"

// Configuration
const int BATTERY_PIN = 34;
const float VOLTAGE_DIVIDER_RATIO = 4.0;
const float ADC_REFERENCE_VOLTAGE = 3.3;
const int ADC_RESOLUTION = 4095;

// Battery voltage thresholds
#if BATTERY_TYPE == LEAD_ACID
  const char* BATTERY_TYPE_NAME = "Lead-Acid";
  const float VOLTAGE_FULL = 12.7;
  const float VOLTAGE_NOMINAL = 12.4;
  const float VOLTAGE_LOW = 12.0;
  const float VOLTAGE_CRITICAL = 11.8;
  const float VOLTAGE_MIN = 10.5;
#elif BATTERY_TYPE == LIFEPO4
  const char* BATTERY_TYPE_NAME = "LiFePO4";
  const float VOLTAGE_FULL = 14.6;
  const float VOLTAGE_NOMINAL = 13.2;
  const float VOLTAGE_LOW = 12.8;
  const float VOLTAGE_CRITICAL = 12.0;
  const float VOLTAGE_MIN = 10.0;
#endif

float calculateBatteryPercentage(float voltage) {
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

float adcToBatteryVoltage(int adcReading) {
  float adcVoltage = (adcReading / (float)ADC_RESOLUTION) * ADC_REFERENCE_VOLTAGE;
  return adcVoltage * VOLTAGE_DIVIDER_RATIO;
}
