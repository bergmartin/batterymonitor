/*
 * Battery Monitor Library
 * 
 * Object-oriented interface for battery voltage monitoring
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>
#include "battery_config.h"
// Runtime battery chemistry selection
enum class BatteryChemistry { LEAD_ACID, LIFEPO4 };

// Battery status enumeration
enum class BatteryStatus {
  FULL,
  GOOD,
  LOW_BATTERY,
  CRITICAL,
  DEAD
};

// Battery reading structure
struct BatteryReading {
  float voltage;
  float percentage;
  BatteryStatus status;
  unsigned long timestamp;
  
  BatteryReading() 
    : voltage(0.0f), percentage(0.0f), status(BatteryStatus::DEAD), timestamp(0) {}
};

// BatteryMonitor class - Main interface for battery monitoring
class BatteryMonitor {
public:
  // Constructor
  BatteryMonitor();
  
  // Initialization
  void begin();
  // Runtime configuration
  static void setChemistry(BatteryChemistry chemistry);
  static BatteryChemistry getChemistry();
  
  // Reading functions
  BatteryReading readBattery();
  float readVoltage();
  
  // Calculation functions
  static float calculatePercentage(float voltage);
  static BatteryStatus determineStatus(float voltage);
  static const char* statusToString(BatteryStatus status);
  static float adcToVoltage(int adcValue);
  
  // Utility functions
  void printReading(const BatteryReading& reading);
  void printStartupInfo();
  
  // Configuration getters
  static const char* getBatteryTypeName();
  static float getMinVoltage();
  static float getMaxVoltage();
  
private:
  // ADC reading function
  int readADC();
};

// Legacy function compatibility (for tests)
float calculateBatteryPercentage(float voltage);
String getBatteryStatus(float voltage);
float adcToBatteryVoltage(int adcReading);

// Legacy constants (for tests)
extern const int BATTERY_PIN;
extern const float VOLTAGE_DIVIDER_RATIO;
extern const float ADC_REFERENCE_VOLTAGE;
extern const int ADC_RESOLUTION;
extern const char* BATTERY_TYPE_NAME;
extern float VOLTAGE_FULL;
extern float VOLTAGE_NOMINAL;
extern float VOLTAGE_LOW;
extern float VOLTAGE_CRITICAL;
extern float VOLTAGE_MIN;

#endif // BATTERY_MONITOR_H
