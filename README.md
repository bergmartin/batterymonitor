# ESP32 Battery Voltage Monitor

A PlatformIO project for monitoring 12V battery voltage using an ESP32 microcontroller with **ultra-low power consumption** through deep sleep mode.

## 🌟 Key Features

- **Ultra-Low Power:** Deep sleep mode with only 0.12-0.23 mA average draw (99.8% power reduction!)
- **Long Battery Life:** Monitor can run for years on the battery it's monitoring
- **MQTT Integration:** Publish readings to MQTT broker for remote monitoring (Home Assistant, Node-RED, etc.)
- Real-time 12V battery voltage monitoring
- Support for **Lead-Acid** and **LiFePO4** battery types
- Battery type configured at build time
- Battery percentage calculation with type-specific thresholds
- Battery status indicators (FULL, GOOD, LOW_BATTERY, CRITICAL, DEAD)
- Visual battery level display
- Low battery warnings
- Averaged ADC readings for accuracy
- RTC memory for persistent data across sleep cycles
- **3D Printable Enclosure** for professional deployment

## ⚡ Power Consumption

| Mode | Current Draw | Battery Life (100Ah) |
|------|-------------|---------------------|
| **Deep Sleep (default)** | 0.12-0.23 mA | **50-95 years** 🎉 |
| Continuous monitoring | 80-160 mA | 26-52 days |

**Deep sleep mode draws less power than the battery loses naturally!**

See [DEEP_SLEEP.md](DEEP_SLEEP.md) for detailed power analysis.

## Hardware Requirements

### Components
- ESP32 development board
- 12V battery
- 30kΩ resistor (R1)
- 10kΩ resistor (R2)
- Connecting wires

### Circuit Diagram

```
12V Battery (+) ----[30kΩ]----+----[10kΩ]---- GND
                               |
                          GPIO34 (ADC)
                          ESP32
```

### Voltage Divider Explanation

The ESP32 ADC can only read voltages up to 3.3V. To safely measure a 12V battery, we use a voltage divider:

- **R1** (30kΩ) + **R2** (10kΩ) creates a 4:1 ratio
- 12V battery voltage becomes ~3V at the ADC pin
- Formula: `V_ADC = V_Battery × (R2 / (R1 + R2))`
- Result: `V_ADC = 12V × (10kΩ / 40kΩ) = 3V`

## Wiring Instructions

1. **Connect Voltage Divider:**
   - Solder 30kΩ resistor (R1) to battery positive terminal
   - Connect 10kΩ resistor (R2) between the junction and GND
   - Connect junction point to GPIO34 on ESP32

2. **Ground Connection:**
   - Connect battery negative terminal to ESP32 GND

3. **Power ESP32:**
   - Power the ESP32 via USB or separate power supply
   - **Important:** Do NOT power ESP32 directly from the 12V battery without a voltage regulator

## Software Setup

### Installation

1. Install [PlatformIO](https://platformio.org/)
2. Clone or download this project
3. Open the project in VS Code with PlatformIO extension

### MQTT Configuration (Optional)

To enable remote monitoring via MQTT:

1. **Configure WiFi credentials:**
   ```bash
   cd include/
   cp wifi_credentials.h.example wifi_credentials.h
   # Edit wifi_credentials.h with your WiFi SSID and password
   ```

2. **Configure MQTT broker:**
   ```bash
   cp mqtt_credentials.h.example mqtt_credentials.h
   # Edit mqtt_credentials.h with your MQTT broker details
   ```

3. **See [MQTT.md](MQTT.md) for complete MQTT setup guide**, including:
   - MQTT broker options (Mosquitto, CloudMQTT, Home Assistant)
   - Topic structure and JSON format
   - Home Assistant integration
   - Testing and troubleshooting

### Build and Upload

**Default (Lead-Acid battery):**
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

**For LiFePO4 battery:**
```bash
# Build for LiFePO4
pio run -e esp32dev -D BATTERY_TYPE=BATTERY_TYPE_LIFEPO4

# Upload to ESP32
pio run -e esp32dev -D BATTERY_TYPE=BATTERY_TYPE_LIFEPO4 --target upload

# Monitor serial output
pio device monitor
```

**Alternative: Edit platformio.ini**

Change the build flag in the default `[env:esp32dev]` section:
```ini
build_flags = 
  -D BATTERY_TYPE=BATTERY_TYPE_LIFEPO4  ; Options: BATTERY_TYPE_LEAD_ACID or BATTERY_TYPE_LIFEPO4
```

Or use PlatformIO IDE buttons in VS Code.

## Testing

The project includes comprehensive unit tests for all battery monitoring logic.

### Run Tests

**Test Lead-Acid configuration:**
```bash
pio test -e esp32dev
```

**Test LiFePO4 configuration:**
```bash
pio test -e esp32dev -D BATTERY_TYPE=BATTERY_TYPE_LIFEPO4
```

**Test both configurations:**
```bash
pio test
```

The tests cover:
- Battery percentage calculations
- Battery status determination
- ADC to voltage conversion
- Voltage divider safety
- Edge cases and boundary conditions

See `test/README.md` for detailed testing documentation.

## Configuration

### Battery Type Selection

The project supports two battery types, configured at **build time**:

**Option 1: Use the default environment with build flag**
- `esp32dev` with default `BATTERY_TYPE_LEAD_ACID` build flag
- `esp32dev` with `-D BATTERY_TYPE=BATTERY_TYPE_LIFEPO4` build flag for LiFePO4

**Option 2: Edit the default environment**

In `platformio.ini`, modify the build_flags:
```ini
[env:esp32dev]
build_flags = 
  -D BATTERY_TYPE=LEAD_ACID  ; or LIFEPO4
```

### Voltage Thresholds

The voltage thresholds are automatically set based on battery type:

**Lead-Acid (12V):**
- Full: 12.7V
- Nominal: 12.4V (75%)
- Low: 12.0V (25%)
- Critical: 11.8V
- Minimum: 10.5V

**LiFePO4 (12V nominal, 4S configuration):**
- Full: 14.6V
- Nominal: 13.2V (75%)
- Low: 12.8V (25%)
- Critical: 12.0V
- Minimum: 10.0V

### Other Adjustable Parameters

You can adjust these parameters in `lib/BatteryMonitor/battery_config.h` (in the `Config` namespace):

```cpp
constexpr int BATTERY_ADC_PIN = 34;           // ADC pin (GPIO34)
constexpr float VOLTAGE_DIVIDER_RATIO = 4.0;  // Adjust if using different resistors
constexpr unsigned long READING_INTERVAL_MS = 10000;  // Reading interval (active mode, 10 seconds)
constexpr uint64_t DEEP_SLEEP_INTERVAL_US = 3600000000ULL;  // Deep sleep interval (1 hour)
```

### For Different Resistor Values

If you use different resistors, calculate the voltage divider ratio:

```
VOLTAGE_DIVIDER_RATIO = (R1 + R2) / R2
```

Example:
- R1 = 100kΩ, R2 = 33kΩ → Ratio = 4.03
- R1 = 20kΩ, R2 = 10kΩ → Ratio = 3.0

## Serial Output Example

**Lead-Acid Battery:**
```
=================================
ESP32 Battery Voltage Monitor
=================================
Battery Type: Lead-Acid
Voltage Range: 10.5V - 12.7V

─────────────────────────────────
Battery Voltage: 12.45 V
Battery Level:   88.6 %
Status:          GOOD
Battery: [████████░░]
```

**LiFePO4 Battery:**
```
=================================
ESP32 Battery Voltage Monitor
=================================
Battery Type: LiFePO4
Voltage Range: 10.0V - 14.6V

─────────────────────────────────
Battery Voltage: 13.84 V
Battery Level:   83.5 %
Status:          GOOD
Battery: [████████░░]
```

## Battery Voltage Guidelines

The project automatically configures voltage thresholds based on your battery type selection.

### Lead-Acid Batteries (12V)
- **12.7V** - Fully charged (100%)
- **12.4V** - 75% charged
- **12.2V** - 50% charged
- **12.0V** - 25% charged
- **11.8V** - Discharged (should recharge)
- **10.5V** - Minimum safe voltage

### LiFePO4 Batteries (12V nominal, 4S configuration)
- **14.6V** - Fully charged (100%) - 4 × 3.65V
- **13.6V** - 90% charged
- **13.2V** - 75% charged
- **13.0V** - 50% charged
- **12.8V** - 25% charged
- **12.0V** - Discharged (should recharge)
- **10.0V** - Minimum safe voltage - 4 × 2.5V

**Note:** LiFePO4 batteries have a flatter discharge curve than lead-acid, maintaining voltage better under load.

## Safety Notes

⚠️ **Important Safety Information:**

1. **Never exceed ADC input voltage:** Ensure your voltage divider outputs max 3.0V
2. **Double-check connections:** Incorrect wiring can damage your ESP32
3. **Use proper resistor wattage:** 1/4W resistors are typically sufficient
4. **Battery polarity:** Connect positive to R1, negative to GND
5. **Isolated ground:** If powering ESP32 separately, ensure common ground with battery

## Calibration

For more accurate readings, you can calibrate the system:

1. Measure actual battery voltage with a multimeter
2. Compare with ESP32 reading
3. Adjust the `VOLTAGE_DIVIDER_RATIO` constant if needed
4. The ADC may have slight variations; fine-tune the ratio for accuracy

## Troubleshooting

**Reading shows 0V:**
- Check voltage divider connections
- Verify GPIO34 is connected to the junction point
- Check resistor values

**Reading too high/low:**
- Verify resistor values (use multimeter)
- Recalculate voltage divider ratio
- Check for loose connections

**Unstable readings:**
- Increase SAMPLES count for more averaging
- Add a small capacitor (0.1µF) across R2
- Check for electromagnetic interference

**MQTT not connecting:**
- See [MQTT.md](MQTT.md) troubleshooting section
- Verify WiFi credentials in `include/wifi_credentials.h`
- Check MQTT broker is accessible and credentials are correct
- Test broker with mosquitto-clients: `mosquitto_sub -h broker -t "#"`

## Documentation

- **[WIRING.md](WIRING.md)** - Detailed wiring instructions with buck converter setup
- **[DEEP_SLEEP.md](DEEP_SLEEP.md)** - Deep sleep implementation and power analysis
- **[MQTT.md](MQTT.md)** - MQTT setup, configuration, and integration guide
- **[REFACTORING.md](REFACTORING.md)** - Code structure and refactoring documentation
- **[test/README.md](test/README.md)** - Testing framework and test documentation
- **[enclosure/README.md](enclosure/README.md)** - 3D printable enclosure guide

## License

MIT License - Feel free to modify and use in your projects.
