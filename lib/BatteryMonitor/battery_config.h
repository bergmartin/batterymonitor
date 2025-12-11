/*
 * Battery Monitor Configuration
 * 
 * Hardware and monitoring configuration for the battery monitor
 */

#ifndef BATTERY_CONFIG_H
#define BATTERY_CONFIG_H

// Battery type definitions
#define LEAD_ACID 1
#define LIFEPO4 2

// Default to Lead-Acid if not specified
#ifndef BATTERY_TYPE
#define BATTERY_TYPE LEAD_ACID
#endif

// Hardware Configuration
namespace Config {
  // ADC Pin Configuration
  constexpr int BATTERY_ADC_PIN = 34;  // GPIO34 (ADC1_CH6)
  
  // ADC Settings
  constexpr int ADC_RESOLUTION_BITS = 12;
  constexpr int ADC_MAX_VALUE = 4095;  // 2^12 - 1
  constexpr float ADC_REFERENCE_VOLTAGE = 3.3;  // Volts
  
  // Voltage Divider Configuration
  // R1 = 30kΩ (high side), R2 = 10kΩ (low side)
  constexpr float VOLTAGE_DIVIDER_RATIO = 4.0;  // (R1 + R2) / R2
  
  // Sampling Configuration
  constexpr int SAMPLE_COUNT = 10;  // Number of ADC samples to average
  constexpr int SAMPLE_DELAY_MS = 10;  // Delay between samples
  
  // Monitoring Configuration
  constexpr unsigned long READING_INTERVAL_MS = 3600000;  // 1 hour (3600 seconds)
  constexpr unsigned long STARTUP_DELAY_MS = 1000;
  constexpr unsigned long SERIAL_BAUD_RATE = 115200;
  
  // Deep Sleep Configuration
  constexpr bool ENABLE_DEEP_SLEEP = true;  // Enable power-saving deep sleep
  constexpr uint64_t DEEP_SLEEP_INTERVAL_US = 3600000000ULL;  // 1 hour in microseconds
  constexpr int AWAKE_TIME_MS = 5000;  // Time to stay awake for reading and display
  
  // Battery Type Specific Thresholds
  #if BATTERY_TYPE == LEAD_ACID
    constexpr char BATTERY_TYPE_NAME[] = "Lead-Acid";
    
    namespace Voltage {
      constexpr float FULL = 12.7;       // 100% - Fully charged
      constexpr float NOMINAL = 12.4;    // ~75% - Good condition
      constexpr float LOW_THRESHOLD = 12.0;        // ~25% - Should recharge soon
      constexpr float CRITICAL = 11.8;   // ~10% - Recharge immediately
      constexpr float MINIMUM = 10.5;    // 0% - Minimum safe voltage
    }
    
  #elif BATTERY_TYPE == LIFEPO4
    constexpr char BATTERY_TYPE_NAME[] = "LiFePO4";
    
    namespace Voltage {
      constexpr float FULL = 14.6;       // 100% - Fully charged (4S = 4 × 3.65V)
      constexpr float NOMINAL = 13.2;    // ~75% - Good condition
      constexpr float LOW_THRESHOLD = 12.8;        // ~25% - Should recharge soon
      constexpr float CRITICAL = 12.0;   // ~10% - Recharge immediately
      constexpr float MINIMUM = 10.0;    // 0% - Minimum safe voltage (4S = 4 × 2.5V)
    }
    
  #else
    #error "Invalid BATTERY_TYPE. Use LEAD_ACID or LIFEPO4"
  #endif
}

#endif // BATTERY_CONFIG_H
