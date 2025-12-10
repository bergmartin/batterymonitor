/*
 * Battery Monitor Library
 * 
 * Core functions for battery voltage monitoring
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

// Battery type definitions
#define LEAD_ACID 1
#define LIFEPO4 2

// Default to Lead-Acid if not specified
#ifndef BATTERY_TYPE
#define BATTERY_TYPE LEAD_ACID
#endif

// Configuration
extern const int BATTERY_PIN;
extern const float VOLTAGE_DIVIDER_RATIO;
extern const float ADC_REFERENCE_VOLTAGE;
extern const int ADC_RESOLUTION;

// Battery voltage thresholds - configured based on battery type
#if BATTERY_TYPE == LEAD_ACID
  extern const char* BATTERY_TYPE_NAME;
  extern const float VOLTAGE_FULL;
  extern const float VOLTAGE_NOMINAL;
  extern const float VOLTAGE_LOW;
  extern const float VOLTAGE_CRITICAL;
  extern const float VOLTAGE_MIN;
#elif BATTERY_TYPE == LIFEPO4
  extern const char* BATTERY_TYPE_NAME;
  extern const float VOLTAGE_FULL;
  extern const float VOLTAGE_NOMINAL;
  extern const float VOLTAGE_LOW;
  extern const float VOLTAGE_CRITICAL;
  extern const float VOLTAGE_MIN;
#else
  #error "Invalid BATTERY_TYPE. Use LEAD_ACID or LIFEPO4"
#endif

// Function declarations
float calculateBatteryPercentage(float voltage);
String getBatteryStatus(float voltage);
float adcToBatteryVoltage(int adcReading);

#endif // BATTERY_MONITOR_H
