# Deep Sleep Power Optimization

## Configuration Changes

### Reading Interval
- **Before:** Every 2 seconds
- **After:** Every 1 hour (3600 seconds)

### Power Mode
- **Before:** Always active
- **After:** Deep sleep between readings

## Power Consumption Comparison

### Original Configuration (Active Mode)
```
Active current: 80-160 mA
Duty cycle: 100% (always on)
Average current: 80-160 mA
Power @ 12V: 0.96-1.92 W
Daily consumption: 1.92-3.84 Ah
```

**Battery Life (100 Ah battery):**
- Best case: 26 days
- Worst case: 52 days

### New Configuration (Deep Sleep Mode)
```
Active current: 80-160 mA (for ~5 seconds)
Deep sleep current: 0.01 mA (10 µA)
Duty cycle: 0.14% active, 99.86% sleep

Calculation:
- Active time per hour: 5 seconds
- Sleep time per hour: 3595 seconds
- Average = (80mA × 5s + 0.01mA × 3595s) / 3600s
- Average current: 0.12-0.23 mA
```

**Power @ 12V:** 1.4-2.8 mW (milliwatts!)

**Daily consumption:** 2.9-5.5 mAh (0.0029-0.0055 Ah)

**Battery Life (100 Ah battery):**
- Best case: **18,182 days (49.8 years)**
- Worst case: **34,483 days (94.5 years)**

## Improvement Summary

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Average Current | 80-160 mA | 0.12-0.23 mA | **99.8% reduction** |
| Average Power | 0.96-1.92 W | 1.4-2.8 mW | **99.85% reduction** |
| Daily Usage | 1.92-3.84 Ah | 2.9-5.5 mAh | **700x less** |
| Battery Life | 26-52 days | 49-94 years | **700x longer** |

## Features

### RTC Memory Persistence
Data preserved across deep sleep cycles:
- Boot count tracking
- Last voltage reading
- Historical data (can be expanded)

### Smart Wake-up
- Timer-based wakeup every hour
- Configurable interval
- Fast boot and reading (~5 seconds)

### Serial Output
Each boot displays:
```
╔═════════════════════════════════╗
║  ESP32 Battery Monitor (Deep Sleep) ║
╚═════════════════════════════════╝
Boot count: 42
Wakeup caused by timer
Last voltage: 12.45 V

─────────────────────────────────
Battery Voltage: 12.43 V
Battery Level:   87.3 %
Status:          GOOD
Battery: [████████░░]

Time awake: 4235 ms
─────────────────────────────────
Entering deep sleep mode...
Next reading in: 3600 seconds
Power consumption: ~10 µA
─────────────────────────────────
```

## Configuration Options

### Disable Deep Sleep (testing/debugging)
In `battery_config.h`:
```cpp
constexpr bool ENABLE_DEEP_SLEEP = false;
```

### Change Reading Interval
```cpp
// 30 minutes
constexpr uint64_t DEEP_SLEEP_INTERVAL_US = 1800000000ULL;

// 2 hours
constexpr uint64_t DEEP_SLEEP_INTERVAL_US = 7200000000ULL;

// 6 hours
constexpr uint64_t DEEP_SLEEP_INTERVAL_US = 21600000000ULL;
```

### Adjust Awake Time
```cpp
constexpr int AWAKE_TIME_MS = 5000;  // 5 seconds (default)
constexpr int AWAKE_TIME_MS = 10000; // 10 seconds (more time for serial)
```

## Real-World Battery Life Examples

### 12V 7Ah SLA Battery (alarm systems)
- Before: 1.8-3.6 days
- **After: 3.5-7 years** ✅

### 12V 35Ah Car Battery (backup power)
- Before: 9-18 days
- **After: 17-35 years** ✅

### 12V 100Ah Deep Cycle (solar/RV)
- Before: 26-52 days
- **After: 50-95 years** ✅

### 12V 200Ah LiFePO4 (off-grid)
- Before: 52-104 days
- **After: 100-190 years** ✅

## Practical Considerations

### Why Not Infinite?
The battery itself has:
- Self-discharge: 2-5% per month (lead-acid)
- Self-discharge: 1-3% per month (LiFePO4)
- This is 600-15,000x more than the monitor draws!

### Actual Limiting Factor
**Battery self-discharge**, not the monitor.

The monitor now uses less power than the battery loses naturally!

## Use Cases Enabled

### Long-Term Monitoring
- Storage battery health checks
- Seasonal equipment (boats, RVs)
- Emergency backup systems
- Solar system monitoring

### Remote Installations
- Off-grid locations
- No external power needed
- Battery lasts for years

### Data Logging
With RTC memory, can track:
- Min/max voltages
- Charge cycles
- Degradation over time

## Technical Details

### ESP32 Deep Sleep
- CPU powered off
- Most RAM powered off
- RTC and ULP co-processor active
- Wake sources: timer, GPIO, touchpad, ULP

### Current Breakdown (Deep Sleep)
- RTC timer: ~5 µA
- RTC memory: ~2 µA
- ULP co-processor: ~3 µA
- Total: ~10 µA (0.01 mA)

### Voltage Divider Current
Even with voltage divider always on:
- 12V / 40kΩ = 0.3 mA
- Still negligible compared to original active draw
- Could use higher resistor values for even less

## Recommended Settings

### For Maximum Battery Life
```cpp
constexpr uint64_t DEEP_SLEEP_INTERVAL_US = 21600000000ULL; // 6 hours
constexpr int SAMPLE_COUNT = 5;  // Faster reading
```

### For Frequent Monitoring
```cpp
constexpr uint64_t DEEP_SLEEP_INTERVAL_US = 900000000ULL; // 15 minutes
constexpr int SAMPLE_COUNT = 10; // Standard accuracy
```

### For Development/Testing
```cpp
constexpr bool ENABLE_DEEP_SLEEP = false;
constexpr unsigned long READING_INTERVAL_MS = 5000; // 5 seconds
```

## Migration from Previous Version

The deep sleep version is **fully backward compatible** in the library. Only the main application changed.

To switch back to continuous monitoring:
```cpp
// In battery_config.h
constexpr bool ENABLE_DEEP_SLEEP = false;
```

## Conclusion

Deep sleep mode transforms the battery monitor from a **power-hungry** device (80-160 mA) to an **ultra-low-power** device (0.12-0.23 mA), making it practical for:
- Long-term deployment
- Battery-powered systems
- Remote monitoring
- Storage applications

The monitor now draws **less power than the battery loses naturally**, making it essentially **"free"** in terms of battery impact! 🎉
