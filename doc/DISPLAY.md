# SH1106 OLED Display Wiring

## Display Specifications
- **Model**: SH1106 OLED Display
- **Resolution**: 128x64 pixels
- **Interface**: I2C
- **Operating Voltage**: 3.3V - 5V
- **I2C Address**: 0x3C (default)

## Wiring Connections

### Default Pin Configuration
| SH1106 Pin | ESP32 Pin | Description |
|-----------|-----------|-------------|
| VCC       | 3.3V      | Power supply (3.3V or 5V) |
| GND       | GND       | Ground |
| SCL       | GPIO 22   | I2C Clock (default) |
| SDA       | GPIO 21   | I2C Data (default) |

### Wiring Diagram
```
ESP32                    SH1106 Display
┌─────────────┐         ┌──────────────┐
│             │         │              │
│     3.3V ───┼─────────┼─── VCC       │
│      GND ───┼─────────┼─── GND       │
│  GPIO 22 ───┼─────────┼─── SCL       │
│  GPIO 21 ───┼─────────┼─── SDA       │
│             │         │              │
└─────────────┘         └──────────────┘
```

## Custom Pin Configuration

If you need to use different I2C pins, you can modify the pin definitions in the code:

### Option 1: Modify DisplayConfig in display_manager.h
```cpp
namespace DisplayConfig {
    const uint8_t I2C_SDA = 21;  // Change to your SDA pin
    const uint8_t I2C_SCL = 22;  // Change to your SCL pin
    const uint8_t I2C_ADDRESS = 0x3C;
    const unsigned long UPDATE_INTERVAL = 1000;
}
```

### Option 2: Pass custom pins during initialization
```cpp
// In main.cpp setup()
display.begin(/* sda */ 21, /* scl */ 22);
```

## Complete Battery Monitor Wiring

When combined with the battery monitor circuit:

```
12V Battery
    │
    ├─── R1 (30kΩ) ─── GPIO 34 (ADC) ─── R2 (10kΩ) ─── GND
    │
    └─── Power Supply (12V to 5V converter)
              │
              └─── ESP32 5V/3.3V Input
                         │
                         ├─── SH1106 VCC
                         ├─── SH1106 GND
                         ├─── SH1106 SCL (GPIO 22)
                         └─── SH1106 SDA (GPIO 21)
```

## Display Features

The display shows:
- **Battery Voltage**: Large display of current voltage
- **Battery Percentage**: Visual progress bar and numeric value
- **Battery Status**: Current state (FULL, GOOD, LOW, CRITICAL, DEAD)
- **WiFi Status**: Connection state and signal strength
- **WiFi RSSI**: Signal strength in dBm with visual indicator
- **Boot Information**: Boot count on startup
- **OTA Status**: Update progress messages
- **Sleep Screen**: Countdown before deep sleep

## Troubleshooting

### Display not working?
1. **Check I2C address**: Some SH1106 displays use 0x3D instead of 0x3C
   - Scan for I2C devices with an I2C scanner sketch
   - Update `I2C_ADDRESS` in `display_manager.h` if different

2. **Check wiring**: Ensure SDA and SCL are not swapped

3. **Check power**: Ensure the display is getting proper 3.3V or 5V

4. **Pull-up resistors**: Most SH1106 modules have built-in pull-ups, but if needed:
   - Add 4.7kΩ resistors from SDA to 3.3V
   - Add 4.7kΩ resistors from SCL to 3.3V

### Serial Monitor Messages
- **Success**: "SH1106 Display initialized"
- **Failure**: "Failed to initialize SH1106 display!"

## Power Consumption Impact

The SH1106 OLED display adds approximately:
- **Active**: 20-30 mA (screen on)
- **Deep Sleep**: Display is powered off during sleep mode

Total power budget:
- **Active with display**: ~100-190 mA (~5-7 seconds)
- **Deep Sleep**: ~0.01 mA (display off, 4 hours)
- **Average impact**: Minimal, still <1 mA average

## Notes

- The display automatically turns off during deep sleep to save power
- Display updates are throttled to 1 second intervals to reduce CPU load
- Battery and WiFi icons provide quick visual status
- All text and graphics are rendered using the U8g2 library
