# ESP32 Battery Monitor - Wiring Guide

This guide describes how to connect and configure the electronic components for the Battery Monitor. There are two primary options to build this project:
1. **Option A: Breadboard / Prototyping Setup**: Using an off-the-shelf ESP32 development board, an external buck converter module (e.g., LM2596), and discrete resistors.
2. **Option B: Custom PCB Design**: Solder SMT components directly onto the custom PCB, featuring an integrated high-efficiency **AP63300 synchronous buck converter** (12V → 3.3V) and an on-board voltage divider.

---

## Option A: Breadboard / Prototyping Setup

### Prototyping Wiring Diagram
```
                    ┌─────────────────┐
12V Battery (+) ────┤ Buck Converter  │
                    │  (12V → 5V)     ├──── 5V ──→ ESP32 VIN
                    └─────────────────┘
                            │
                            └──────────────→ GND ──→ ESP32 GND
                                                      
12V Battery (+) ────[100kΩ R1]───┬────[22kΩ R2]──── GND
                                 │
                           GPIO34 (ESP32)

12V Battery (−) ───────────────────────────────────→ Common GND
```

### Option A Required Components
| Component | Specification | Quantity | Notes |
|-----------|--------------|----------|-------|
| ESP32 Development Board | NodeMCU or similar with GPIO34 | 1 | With built-in USB/Serial & 3.3V LDO |
| Buck Converter Module | LM2596 or similar, 12V→5V | 1 | Adjustable output recommended |
| Resistor R1 | 100kΩ, 1/4W or higher | 1 | For voltage divider (high side) |
| Resistor R2 | 22kΩ, 1/4W or higher | 1 | For voltage divider (low side) |
| Fuse | 1A, automotive blade type | 1 | Safety protection |
| Fuse Holder | Inline blade fuse holder | 1 | For easy replacement |

### Option A Step-by-Step Instructions

#### 1. Power Supply Circuit Setup:
* Connect the battery positive (+) terminal through the 1A fuse to the buck converter input (`IN+`).
* Connect the battery negative (-) terminal to the buck converter input (`IN-`).
* **Critical:** Use a multimeter to measure the output of the buck converter. Adjust the onboard potentiometer until the output reads exactly **5.0V** before connecting it to the ESP32.
* Connect the buck converter output (`OUT+`) to the ESP32 `VIN` (or `5V`) pin.
* Connect the buck converter output (`OUT-`) to the ESP32 `GND` pin.

#### 2. Voltage Divider Setup:
* Connect the battery positive (+) terminal to one end of the 100kΩ resistor ($R_1$).
* Connect the other end of $R_1$ to a junction point.
* Connect the junction point to the ESP32 analog input pin (**GPIO34**).
* Connect a 22kΩ resistor ($R_2$) between the junction point and the common `GND`.
* The divider divides the battery voltage down by a ratio of $5.545:1$ (safe $2.16\text{V}$ for $12\text{V}$ input, max $2.63\text{V}$ for fully-charged LiFePO4 battery at $14.6\text{V}$).

---

## Option B: Custom PCB Design (Recommended)

The custom PCB (located in the [pcb/](file:///home/martin/projects/batterymonitor/pcb) directory) integrates all power management, sensing, and display routing onto a single compact board.

### Custom PCB Architecture Block Diagram
```
                        Custom PCB (All-in-One)
                    ┌──────────────────────────────────┐
                    │  12V Input (J1 Screw Terminal)   │
                    │         │                        │
                    │         ├──→ [AP63300 Buck]      │
                    │         │    (12V → 3.3V)        │
                    │         │         │              │
                    │         │         └──→ 3.3V ───┐ │
                    │         │                      │ │
                    │         └─→ [Divider R1/R2]    │ │
                    │                   │            │ │
                    │                 GPIO34         │ │
                    │                   │            │ │
                    │                   ▼            ▼ │
                    │           [ESP32-WROOM-32U module]
                    │                   │              │
                    │                   └──→ [J2 OLED] │
                    └─────────────────────────│────────┘
                                              ▼
                                      SH1106 OLED (I2C)
```

### Integrated AP63300 Synchronous Buck Converter
Instead of using an external bulk module that regulates to 5V, the PCB integrates a **Diodes Inc. AP63300** synchronous buck converter (`U1`).
* **High Efficiency**: Operates at $>90\%$ efficiency, generating minimal heat compared to linear regulators (like the AMS1117 which wastes energy as heat).
* **Direct 3.3V Power**: Steps the battery's 12V input directly down to **3.3V**, powering the ESP32 module (`U2`) directly.
* **Low Noise (EMI)**: Features a frequency spread spectrum (FSS) clocking system operating at 500kHz to reduce electromagnetic interference.

#### AP63300 Supporting Passive Network
The PCB utilizes the following optimized component layout:
* **Inductor (`L1`)**: A $4.7\,\mu\text{H}$ Sunlord molded power inductor (`MPL2016S4R7MHT`, LCSC Part: `C98335`) in an 0806 package, selected for optimal switching performance.
* **Bootstrap Capacitor (`C4`)**: A $100\,\text{nF}$ 50V X7R ceramic capacitor (`C49678`) connected between the `SW` (Switching) and `BST` (Bootstrap) pins to drive the high-side MOSFET.
* **Feedback Network (`R3`, `R4`)**:
  * $R_3 = 100\,\text{k}\Omega$ 1% 0805 (`C149504`)
  * $R_4 = 32\,\text{k}\Omega$ 1% 0805 (`C182555`)
  * Based on the feedback reference voltage ($V_{\text{fb}} = 0.8\text{V}$), this resistor ratio delivers exactly $3.30\text{V}$ output:
    $$V_{\text{out}} = 0.8\text{V} \times \left(1 + \frac{R_3}{R_4}\right) = 0.8\text{V} \times \left(1 + \frac{100\,\text{k}\Omega}{32\,\text{k}\Omega}\right) = 3.30\text{V}$$
* **Input/Output Filtering (`C1`, `C2`)**:
  * Input capacitor $C_1$: $10\,\mu\text{F}$ 25V X5R ceramic capacitor (`C40894`) to handle voltage ripples.
  * Output capacitor $C_2$: $22\,\mu\text{F}$ 10V X5R ceramic capacitor (`C89572`) to stabilize the 3.3V rail.

### Custom PCB Step-by-Step Instructions

#### 1. Assemble and Solder Components:
* Solder the SMT components in order of height (starting with resistors/capacitors, then the AP63300 IC, inductor, and finally the ESP32 module).
* Ensure correct alignment of pin 1 for the AP63300 (`U1`) and the ESP32-WROOM-32U (`U2`).

#### 2. Connect the Input:
* Connect the battery positive (+) terminal through an inline 1A fuse to pin 1 of the screw terminal block `J1` (labeled `12V`).
* Connect the battery negative (-) terminal to pin 2 of `J1` (labeled `GND`).

#### 3. Connect the Display:
* Connect the SH1106 OLED I2C display to the 4-pin female socket/header `J2`. Pinout mappings are:
  * **Pin 1 (3.3V)** → OLED VCC
  * **Pin 2 (GND)** → OLED GND
  * **Pin 3 (SCL)** → OLED SCL
  * **Pin 4 (SDA)** → OLED SDA

---

## Detailed Pin Mappings & Configuration

### ESP32 Pin Assignment
| ESP32 Pin | Function | Schematic Net | Connection / Purpose |
|-----------|----------|---------------|----------------------|
| **GPIO34** | ADC Input (ADC1_CH6) | `V_ADC` | Voltage divider junction (Reads battery voltage) |
| **GPIO21** | I2C SDA | `SDA` | SH1106 OLED Display Data Line |
| **GPIO22** | I2C SCL | `SCL` | SH1106 OLED Display Clock Line |
| **3.3V** | Power Input | `3.3V` | Main power rail from buck converter |
| **GND** | Ground | `GND` | Common ground reference |

### Voltage Divider Formula & Scaling
The voltage divider uses $R_1 = 100\,\text{k}\Omega$ and $R_2 = 22\,\text{k}\Omega$.
$$V_{\text{ADC}} = V_{\text{Battery}} \times \left(\frac{R_2}{R_1 + R_2}\right) = V_{\text{Battery}} \times \left(\frac{22\,\text{k}\Omega}{122\,\text{k}\Omega}\right)$$
This yields a voltage scaling ratio of **$5.545$**.
* At $12.0\text{V}$ battery voltage, the ADC pin receives $2.16\text{V}$.
* At $14.6\text{V}$ (maximum charge for a 12V LiFePO4 battery), the ADC pin receives $2.63\text{V}$, which is safely under the ESP32's $3.3\text{V}$ limit.

---

## Testing & Calibration Procedure

### 1. Power Supply Test (Option A Only)
1. Connect the battery to the buck converter (with fuse).
2. Measure the buck converter output: should read `5.0V ± 0.1V`.
3. Adjust the converter output potentiometer if necessary.
4. Disconnect the battery.

### 2. Voltage Divider Test
1. Connect the voltage divider (or completed PCB) to the battery.
2. Measure the voltage at the junction point (where GPIO34 connects).
3. It should read `~3.0V` when connected to a 12V battery.
4. Disconnect the battery.

### 3. Complete System Test
1. Solder or connect all components.
2. Upload the firmware to the ESP32.
3. Connect the battery and observe the serial/display output.
4. Verify the voltage reading matches your multimeter reading (within `0.1V`).

---

## Wire Gauge & Color Coding Recommendations

### Wire Gauge
* **Power Lines (Battery to Board)**: Use **18-20 AWG** stranded wire. Red for positive, black for negative.
* **Signal Lines**: Use **22-24 AWG** stranded wire for low-current signals (<1 mA).

### Standard Colors
* **Red**: Battery positive (+), 5V/3.3V power lines
* **Black**: Ground (GND), battery negative (−)
* **Yellow/Orange**: Signal wires (e.g., I2C lines, ADC line)

---

## Safety Considerations

### Fuse Protection
* **Warning:** Always place a **1A inline fuse** on the positive line close to the battery. In case of a short circuit in the wiring or on the PCB, a 12V lead-acid or LiFePO4 battery can dump hundreds of amperes, creating a severe fire hazard.

### Grounding
* **Common Ground**: All ground connections (Battery negative, Buck converter ground, ESP32 ground, and Display ground) must be connected to a single reference net (`GND`).

### PCB Thermal Management
* Because the AP63300 is a highly efficient synchronous buck switching converter, it remains cool to the touch during normal operation. If the converter IC or inductor becomes hot, immediately disconnect power and check for a solder bridge or short circuit on the board.

---

## Troubleshooting

### ESP32 Won't Power On
* Check buck converter output voltage (5V for Option A, 3.3V for Option B).
* Verify ground connections.
* Check fuse continuity.
* Measure battery voltage (should be >10.5V).

### Wrong Voltage Readings
* Verify resistor values with a multimeter.
* Check for loose connections at the junction point or terminal blocks.
* Ensure GPIO34 pin is used (not another GPIO).
* Adjust the `VOLTAGE_DIVIDER_RATIO` constant in your software configurations.

### Intermittent Readings
* Check for loose wires, especially at junctions.
* Add a small capacitor (0.1µF) across $R_2$ for noise filtering and stability.
* Verify a solid common ground connection.

---

## Mounting & Enclosure Selection

### Mounting Options
* **Breadboard (Prototyping)**: Best for temporary testing, not suitable for permanent deployment.
* **Perfboard (Semi-Permanent)**: Solder components for reliability. Mount ESP32 with pin headers.
* **PCB (Permanent)**: Solder components directly to the custom PCB for a robust, professional design.

### Enclosure Selection
* **Indoor Use**: Basic plastic project box with ventilation holes.
* **Outdoor/Automotive Use**: Use an IP65+ rated weatherproof enclosure with cable glands to seal wire entry points.

---

## Power Consumption from Battery
With a standard deep-sleep configuration (e.g., 1-hour sleep interval):

| State | Current Draw (from 12V) | Time per Day |
|-------|------------------------|--------------|
| Active | ~50 mA | 30 seconds |
| Sleep | ~1-5 mA | 23h 59m 30s |
| **Average** | **~1-5 mA** | **24 hours** |

* **Daily consumption: ~1.3 mAh from 12V battery**

---

## Additional Resources

### Datasheets
* ESP32: https://www.espressif.com/en/products/socs/esp32
* AP63300 Buck Converter: https://www.diodes.com/assets/Datasheets/AP63300.pdf
* Sunlord Inductor (MPL2016S4R7MHT): http://www.sunlordinc.com
