# BatteryMonitor Library Examples

This directory contains example sketches demonstrating different uses of the BatteryMonitor library.

## Available Examples

### 1. Basic Monitoring (main.cpp)
The default application in `src/main.cpp` demonstrates basic usage:
- Simple setup and initialization
- Periodic voltage readings
- Automatic status display
- Built-in warnings

**Use this when:** You want simple, out-of-the-box battery monitoring.

### 2. Advanced Monitoring (advanced_monitoring.cpp)
Demonstrates advanced features:
- Data logging and statistics
- Custom alert thresholds
- Running averages
- Min/max voltage tracking
- Session statistics

**Use this when:** You need data analysis, custom alerts, or historical tracking.

## Using an Example

### Method 1: Copy to main.cpp
```bash
cp examples/advanced_monitoring.cpp src/main.cpp
pio run --target upload
```

### Method 2: Reference in your own code
Include the library and use the examples as reference:

```cpp
#include "battery_monitor.h"

BatteryMonitor monitor;

void setup() {
  Serial.begin(115200);
  monitor.begin();
  monitor.printStartupInfo();
}

void loop() {
  BatteryReading reading = monitor.readBattery();
  monitor.printReading(reading);
  delay(2000);
}
```

## Key Concepts

### BatteryReading Structure
```cpp
struct BatteryReading {
  float voltage;          // Battery voltage in volts
  float percentage;       // Battery level 0-100%
  BatteryStatus status;   // FULL, GOOD, LOW_BATTERY, CRITICAL, DEAD
  unsigned long timestamp; // millis() when reading was taken
};
```

### BatteryStatus Enum
```cpp
enum class BatteryStatus {
  FULL,        // >= VOLTAGE_FULL
  GOOD,        // >= VOLTAGE_NOMINAL
  LOW_BATTERY, // >= VOLTAGE_LOW_THRESHOLD
  CRITICAL,    // >= VOLTAGE_CRITICAL
  DEAD         // < VOLTAGE_CRITICAL
};
```

### Static Helper Functions
```cpp
// Calculate percentage from voltage
float percent = BatteryMonitor::calculatePercentage(12.5);

// Determine status from voltage
BatteryStatus status = BatteryMonitor::determineStatus(12.5);

// Convert status to string
const char* str = BatteryMonitor::statusToString(status);

// Convert ADC reading to voltage
float voltage = BatteryMonitor::adcToVoltage(3500);
```

## Configuration

All examples use the configuration from `battery_config.h`. To modify:

1. **Change reading interval:**
   ```cpp
   // In your code, not in config
   const unsigned long MY_INTERVAL = 5000;  // 5 seconds
   ```

2. **Adjust sample count:**
   Modify `Config::SAMPLE_COUNT` in `battery_config.h`

3. **Change battery type:**
   Set in `platformio.ini`:
   ```ini
   build_flags = -D BATTERY_TYPE=LIFEPO4
   ```

## Custom Alerts Example

```cpp
void checkVoltageAlert(const BatteryReading& reading) {
  const float WARNING_VOLTAGE = 11.8;
  
  if (reading.voltage < WARNING_VOLTAGE) {
    // Send notification, trigger relay, etc.
    digitalWrite(LED_PIN, HIGH);
  }
}
```

## Data Logging Example

```cpp
void logToSD(const BatteryReading& reading) {
  File logFile = SD.open("/battery.log", FILE_APPEND);
  logFile.print(reading.timestamp);
  logFile.print(",");
  logFile.print(reading.voltage, 3);
  logFile.print(",");
  logFile.println(reading.percentage, 1);
  logFile.close();
}
```

## Tips

1. **Sampling:** More samples = more accurate but slower readings
2. **Timing:** Use non-blocking code in loop() for responsive monitoring
3. **Filtering:** Consider implementing a moving average for smoother readings
4. **Calibration:** Use a multimeter to verify voltage readings and adjust if needed
5. **Temperature:** Battery voltage varies with temperature - consider compensation

## Troubleshooting

**Readings seem noisy:**
- Increase `Config::SAMPLE_COUNT`
- Add capacitor across voltage divider output
- Check for loose connections

**Voltage reads too high/low:**
- Verify voltage divider resistor values
- Check `Config::VOLTAGE_DIVIDER_RATIO`
- Measure actual resistor values with multimeter

**Status doesn't match expected:**
- Adjust voltage thresholds in `battery_config.h`
- Different battery types have different voltage curves
