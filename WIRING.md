# ESP32 Battery Monitor - Wiring Guide

## Complete Wiring Diagram

```
                    ┌─────────────────┐
12V Battery (+) ────┤ Buck Converter  │
                    │  (12V → 5V)     ├──── 5V ──→ ESP32 VIN
                    └─────────────────┘
                            │
                            └──────────────→ GND ──→ ESP32 GND
                                                      
12V Battery (+) ────[30kΩ R1]────┬────[10kΩ R2]──── GND
                                 │
                           GPIO34 (ESP32)

12V Battery (−) ───────────────────────────────────→ Common GND
```

## Required Components

### Electronic Components
| Component | Specification | Quantity | Notes |
|-----------|--------------|----------|-------|
| ESP32 Development Board | Any variant with GPIO34 | 1 | With built-in USB/Serial |
| Buck Converter | LM2596 or similar, 12V→5V | 1 | Adjustable output recommended |
| Resistor R1 | 30kΩ, 1/4W or higher | 1 | For voltage divider (high side) |
| Resistor R2 | 10kΩ, 1/4W or higher | 1 | For voltage divider (low side) |
| Fuse | 1A, automotive blade type | 1 | Safety protection |
| Fuse Holder | Inline blade fuse holder | 1 | For easy replacement |

### Wiring & Connectors
| Item | Specification | Length | Purpose |
|------|--------------|--------|---------|
| Power Wire | 18-20 AWG stranded | 1-2m | Battery to buck converter |
| Signal Wire | 22-24 AWG stranded | 0.5m | Buck to ESP32, voltage divider |
| Heat Shrink Tubing | Assorted sizes | As needed | Wire insulation |
| Terminal Blocks | 2-position screw terminal | 2-3 | Secure connections |
| Connector (optional) | Anderson Powerpole or XT60 | 1 set | Quick disconnect |

### Optional but Recommended
- Breadboard or perfboard for permanent mounting
- Project enclosure (weatherproof if outdoors)
- LED indicator (with 220Ω resistor) for power/activity
- Reverse polarity protection diode (1N5819 or similar)
- Small capacitor (100µF) near ESP32 VIN for stability

## Detailed Wiring Instructions

### Step 1: Power Supply Circuit

**Buck Converter Setup:**
```
1. Battery Positive (+) → Fuse (1A) → Buck Converter IN+
2. Battery Negative (−) → Buck Converter IN−
3. Adjust buck converter output to 5.0V (use multimeter)
4. Buck Converter OUT+ → ESP32 VIN pin
5. Buck Converter OUT− → ESP32 GND pin
```

**Important:** Test the buck converter output voltage BEFORE connecting to ESP32!

### Step 2: Voltage Divider Circuit

**Resistor Connection:**
```
1. Battery Positive (+) → 30kΩ Resistor (R1)
2. Other end of R1 → Junction point (connect to GPIO34)
3. Junction point → 10kΩ Resistor (R2)
4. Other end of R2 → GND (common ground)
```

**Voltage Divider Formula:**
```
V_GPIO34 = V_Battery × (R2 / (R1 + R2))
         = 12V × (10kΩ / 40kΩ)
         = 3.0V  (safe for ESP32's 3.3V ADC)
```

### Step 3: Ground Connection

**Critical:** All grounds must be connected together:
```
Battery (−) = Buck Converter GND = ESP32 GND = Voltage Divider GND
```

Use a common ground point (star grounding preferred).

### Step 4: GPIO Connection

**ESP32 Pin Mapping:**
```
GPIO34 (ADC1_CH6) ← Voltage Divider Junction Point
VIN (5V)          ← Buck Converter OUT+
GND               ← Common Ground
```

**Note:** GPIO34 is input-only, perfect for ADC reading.

## Pin Configuration Reference

### ESP32 Pins Used
| Pin | Function | Connection |
|-----|----------|------------|
| GPIO34 | ADC Input | Voltage divider junction (3V max) |
| VIN | Power Input | Buck converter 5V output |
| GND | Ground | Common ground |

### Buck Converter Pins
| Pin | Connection |
|-----|------------|
| IN+ | Battery + (through fuse) |
| IN− | Battery − (ground) |
| OUT+ | ESP32 VIN (5V) |
| OUT− | ESP32 GND |

## Wire Gauge Recommendations

### Power Lines (12V Battery to Buck Converter)
- **18-20 AWG** stranded wire
- Red for positive, black for negative
- Maximum length: 2 meters (to minimize voltage drop)
- Current capacity: Up to 200 mA needed

### Signal Lines (Buck to ESP32, Voltage Divider)
- **22-24 AWG** stranded wire
- Any color (use red/black for clarity)
- Keep as short as practical
- Low current (<1 mA for voltage divider)

### Ground Wire
- **18-20 AWG** (same as power)
- Must be capable of carrying full current
- Keep length minimal for stable ground reference

## Safety Considerations

### Fuse Protection
```
Battery (+) ──[1A Fuse]── Buck Converter
```
- Protects against short circuits
- Use automotive blade fuse for easy replacement
- 1A rating is sufficient for ESP32 + margin

### Reverse Polarity Protection (Optional)
```
Battery (+) ──[Diode Anode→Cathode]── Buck Converter IN+
```
- Use Schottky diode (1N5819) - low voltage drop
- Prevents damage if battery connected backwards
- Small voltage drop (~0.3V) acceptable for 12V input

### Voltage Divider Protection
- Resistors prevent excessive current to GPIO
- Even if ESP32 fails, max current = 12V / 40kΩ = 0.3 mA
- Well within safe limits

## Connection Checklist

Before powering on, verify:

- [ ] Buck converter adjusted to 5.0V output (measured with multimeter)
- [ ] All ground connections are secure and common
- [ ] Voltage divider resistors are correct values (30kΩ and 10kΩ)
- [ ] No shorts between power and ground
- [ ] Fuse is installed in positive line
- [ ] Battery polarity is correct (+ to +, − to −)
- [ ] ESP32 is not connected yet (test voltages first)
- [ ] Junction voltage is ~3V when connected to 12V battery

## Testing Procedure

### 1. Power Supply Test
```
1. Connect battery to buck converter (with fuse)
2. Measure buck converter output: should be 5.0V ± 0.1V
3. Adjust if necessary using potentiometer on buck converter
4. Disconnect battery
```

### 2. Voltage Divider Test
```
1. Connect voltage divider to battery
2. Measure voltage at junction point (where GPIO34 connects)
3. Should read ~3.0V with 12V battery
4. Calculate: V_measured × 4 should equal battery voltage
5. Disconnect battery
```

### 3. Complete System Test
```
1. Connect all components
2. Upload code to ESP32
3. Connect battery and observe serial output
4. Verify voltage reading matches multimeter (within 0.1V)
```

## Troubleshooting

### ESP32 Won't Power On
- Check buck converter output voltage (5V)
- Verify ground connections
- Check fuse continuity
- Measure battery voltage (should be 10.5V+)

### Wrong Voltage Readings
- Verify resistor values with multimeter
- Check for loose connections at junction point
- Ensure GPIO34 pin is used (not another GPIO)
- Recalibrate voltage divider ratio in config if needed

### Intermittent Readings
- Check for loose wires, especially at junction
- Add small capacitor (0.1µF) across R2 for stability
- Verify good ground connection

### Buck Converter Gets Hot
- Normal if slightly warm, but shouldn't be too hot to touch
- Check for shorts or excessive current draw
- Ensure proper heat sinking if available

## Wire Color Coding Recommendations

### Standard Colors
- **Red:** Battery positive (+), 5V power
- **Black:** Ground (GND), battery negative (−)
- **Yellow/Orange:** Signal wires (voltage divider to GPIO34)
- **Blue:** Optional indicator LEDs

### Labeling
Use labels or tape markers to identify:
- "BATT +" and "BATT −" at battery end
- "ESP32 VIN" and "ESP32 GND" at ESP32 end
- "GPIO34" at voltage divider junction

## Mounting Recommendations

### Breadboard (Prototyping)
- Temporary setup for testing
- Easy to modify
- Not suitable for permanent installation

### Perfboard (Semi-Permanent)
- Solder components for reliability
- Mount ESP32 with header pins
- Can be enclosed in project box

### PCB (Permanent)
- Design custom PCB for clean installation
- Include all components on single board
- Professional appearance and reliability

## Enclosure Selection

### Indoor Use
- Basic plastic project box
- Ventilation holes for heat dissipation
- Access hole for USB programming

### Outdoor/Automotive Use
- IP65+ rated weatherproof enclosure
- Cable glands for wire entry
- UV-resistant materials
- Consider temperature extremes

## Power Consumption from Battery

With current configuration (4-hour deep sleep):

| State | Current Draw (from 12V) | Time per Day |
|-------|------------------------|--------------|
| Active | ~50 mA | 30 seconds |
| Sleep | ~1-5 mA | 23h 59m 30s |
| **Average** | **~1-5 mA** | **24 hours** |

**Daily consumption: ~1.3 mAh from 12V battery**

## Additional Resources

### Datasheets
- ESP32: https://www.espressif.com/en/products/socs/esp32
- LM2596: Buck converter datasheet
- Resistor specifications: Check manufacturer data

### Tools Needed
- Multimeter (for voltage measurements)
- Soldering iron (for permanent connections)
- Wire strippers
- Screwdriver set
- Heat gun (for heat shrink tubing)

## Final Notes

⚠️ **Safety First:**
- Always disconnect battery before making wiring changes
- Double-check polarity before connecting
- Use appropriate wire gauge for current capacity
- Ensure all connections are secure and insulated
- Test with multimeter before powering ESP32

✅ **Best Practices:**
- Keep wires as short as practical
- Use strain relief for wire connections
- Label all wires for future maintenance
- Document any modifications
- Test thoroughly before permanent installation

---

For questions or issues, refer to the main README.md or open an issue on GitHub.
