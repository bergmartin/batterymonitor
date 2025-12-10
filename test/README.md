# Battery Monitor Tests

This directory contains unit tests for the ESP32 Battery Monitor project using the PlatformIO Unity testing framework.

## Test Coverage

The test suite (`test_battery_monitor.cpp`) covers:

### Battery Percentage Calculation
- Full battery (100%)
- Above full voltage (capped at 100%)
- Minimum voltage (0%)
- Below minimum voltage (capped at 0%)
- Mid-range voltage (~50%)
- Nominal voltage
- Negative voltage handling
- Very high voltage handling

### Battery Status Determination
- FULL status
- GOOD status
- LOW status
- CRITICAL status
- DEAD status
- Boundary conditions between states
- Negative and zero voltage handling

### ADC to Voltage Conversion
- Zero ADC reading
- Maximum ADC reading (4095)
- Mid-range ADC reading
- Typical 12V battery reading
- Voltage divider ratio verification
- Safety verification (ADC input < 3.3V)

### Battery Type Configuration
- Correct threshold values for Lead-Acid
- Correct threshold values for LiFePO4
- Threshold ordering validation

## Running Tests

### Run Tests for Lead-Acid Battery
```bash
# Run tests on native platform (no hardware required)
pio test

# Run tests on ESP32 hardware
pio test -e esp32dev-leadacid
```

### Run Tests for LiFePO4 Battery
```bash
# Run tests with LiFePO4 configuration
pio test -e esp32dev-lifepo4
```

### Run Specific Test
```bash
# Run only battery percentage tests
pio test --filter test_battery_percentage_*

# Run only status tests
pio test --filter test_battery_status_*
```

## Test Output Example

```
======================================
Testing Battery Monitor - Lead-Acid
======================================

test/test_battery_monitor.cpp:77:test_battery_percentage_full [PASSED]
test/test_battery_monitor.cpp:82:test_battery_percentage_above_full [PASSED]
test/test_battery_monitor.cpp:87:test_battery_percentage_minimum [PASSED]
test/test_battery_monitor.cpp:92:test_battery_percentage_below_minimum [PASSED]
test/test_battery_monitor.cpp:97:test_battery_percentage_mid_range [PASSED]
test/test_battery_monitor.cpp:110:test_battery_percentage_nominal [PASSED]
...

-----------------------
32 Tests 0 Failures 0 Ignored 
OK
```

## Writing New Tests

To add new tests, follow this pattern:

```cpp
void test_your_test_name() {
  // Arrange - set up test data
  float testVoltage = 12.5;
  
  // Act - call function under test
  float result = calculateBatteryPercentage(testVoltage);
  
  // Assert - verify expected behavior
  TEST_ASSERT_FLOAT_WITHIN(1.0, 85.0, result);
}
```

Then add it to the `setup()` function:
```cpp
RUN_TEST(test_your_test_name);
```

## Test Assertions Available

- `TEST_ASSERT_EQUAL_FLOAT(expected, actual)`
- `TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)`
- `TEST_ASSERT_EQUAL_STRING(expected, actual)`
- `TEST_ASSERT_GREATER_THAN(threshold, actual)`
- `TEST_ASSERT_LESS_THAN(threshold, actual)`
- `TEST_ASSERT_TRUE(condition)`
- `TEST_ASSERT_FALSE(condition)`

## Known Issues

### LiFePO4 Voltage Divider Warning

When testing with `BATTERY_TYPE=LIFEPO4`, the voltage divider safety test may show a warning:

```
test_voltage_divider_safety: Max ADC voltage (3.65V) exceeds safe limit (3.3V)
```

This is expected behavior. The current 4:1 voltage divider ratio is optimized for Lead-Acid batteries. For LiFePO4 batteries (max 14.6V), a higher ratio (5:1 or 6:1) is recommended for maximum safety margin.

**Recommended fix for LiFePO4:**
- Use R1 = 47kΩ, R2 = 10kΩ (ratio = 5.7)
- This brings 14.6V down to 2.56V (safe for ADC)

## Continuous Integration

These tests can be integrated into CI/CD pipelines:

```yaml
# Example GitHub Actions
- name: Run PlatformIO Tests
  run: |
    pio test -e esp32dev-leadacid
    pio test -e esp32dev-lifepo4
```

## Troubleshooting

**Tests fail to compile:**
- Ensure PlatformIO is up to date: `pio upgrade`
- Check that Unity is installed: `pio lib install "Unity"`

**Tests timeout on hardware:**
- Increase test timeout in platformio.ini: `test_timeout = 120`
- Check serial connection and baud rate

**Float comparison failures:**
- Use `TEST_ASSERT_FLOAT_WITHIN()` instead of exact equality
- Adjust tolerance (delta) as needed for precision
