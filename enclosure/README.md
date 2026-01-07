# 3D Printable Enclosure

This directory contains files for creating a 3D printable enclosure for the ESP32 Battery Monitor.

## Files

- `battery_monitor_bottom.scad` - Bottom enclosure part (with mounting posts)
- `battery_monitor_lid.scad` - Lid part (with ventilation)
- `battery_monitor_enclosure.scad` - Combined design file (legacy, optional)
- `README.md` - This documentation

## Quick Start

### Option 1: Use OpenSCAD (Recommended)

1. **Install OpenSCAD** (free): https://openscad.org/

2. **Generate Bottom Part:**
   - Open `battery_monitor_bottom.scad`
   - Press F5 to preview
   - Press F6 to render
   - File > Export > Export as STL
   - Save as `battery_monitor_bottom.stl`

3. **Generate Lid Part:**
   - Open `battery_monitor_lid.scad`
   - Press F5 to preview
   - Press F6 to render
   - File > Export > Export as STL
   - Save as `battery_monitor_lid.stl`

4. **Customize** dimensions if needed (edit parameters at top of both files)
   - **Important:** Keep dimensions consistent between both files!

### Option 2: Download Pre-made STL Files

STL files are not included in the repository (they're large). Generate them using OpenSCAD or request them.

### Option 3: Use Online CAD Tools

Import the specifications below into:
- **Tinkercad** (https://tinkercad.com) - Web-based, beginner-friendly
- **Fusion 360** (free for hobbyists)
- **FreeCAD** (free, open source)

## Enclosure Specifications

### Dimensions
- **Outer:** 100mm × 70mm × 40mm (L×W×H)
- **Inner:** 95mm × 65mm × 37.5mm
- **Wall thickness:** 2.5mm
- **Corner radius:** 3mm (rounded edges)

### Features
- ESP32 mounting posts with M3 screw holes (4 posts)
- Cable entry holes (8mm diameter)
- Ventilation slots in lid (6 slots)
- Mounting holes for external mounting (M3, in corners)
- Wire management clips
- LED indicator hole (5mm)
- Label area on bottom
- Snap-fit lip design

### ESP32 Board Support
- Fits standard ESP32 dev boards (51mm × 28mm)
- Adjustable post positions for other boards
- 5mm mounting post height
- 3mm clearance under board

## 3D Printing Settings

### Recommended Settings
```
Layer Height:        0.2mm
Shell Thickness:     3 perimeters (1.2mm)
Top/Bottom Layers:   4-5 layers
Infill:              20%
Infill Pattern:      Grid or Gyroid
Support:             Yes (for mounting posts)
Support Overhang:    45°
Bed Adhesion:        Brim (5mm)
Material:            PLA or PETG
Nozzle Temperature:  200-210°C (PLA), 230-240°C (PETG)
Bed Temperature:     60°C (PLA), 80°C (PETG)
Print Speed:         50mm/s
```

### Material Recommendations

| Material | Indoor Use | Outdoor Use | Temperature Range | Notes |
|----------|-----------|-------------|-------------------|-------|
| **PLA** | ✅ Excellent | ❌ Not recommended | 5-40°C | Easy to print, rigid |
| **PETG** | ✅ Excellent | ✅ Good | -20-60°C | More durable, flexible |
| **ABS** | ✅ Good | ✅ Excellent | -40-80°C | Requires heated enclosure |
| **ASA** | ✅ Good | ✅ Excellent | -40-80°C | UV resistant, like ABS |

**For automotive/outdoor:** Use PETG or ASA  
**For indoor/desktop:** PLA is fine

### Print Time & Material
- **Bottom part:** ~1.5-2 hours, ~25g material
- **Lid part:** ~45-60 minutes, ~15g material
- **Total:** ~2.5-3 hours, ~40g material (~$0.80 in filament)

## Assembly Instructions

### Required Hardware
- 4× M3 × 6mm screws (ESP32 mounting)
- 4× M3 × 10mm screws (lid mounting, optional)
- 4× M3 × 20mm screws + nuts (external mounting, optional)

### Step-by-Step Assembly

1. **Print Parts**
   - Print bottom and lid separately
   - Remove supports carefully from mounting posts
   - Clean any stringing or artifacts

2. **Post-Processing**
   - Test M3 screws in mounting post holes
   - If tight, use 2.5mm drill bit to clear holes
   - Sand any rough edges if desired

3. **Mount ESP32**
   - Place ESP32 dev board on mounting posts
   - Align mounting holes
   - Secure with 4× M3 × 6mm screws
   - Don't overtighten (can crack plastic)

4. **Connect Wiring**
   - Thread wires through cable entry holes
   - Use wire management clips to secure
   - Connect all components as per [WIRING.md](../docs/WIRING.md)
   - Ensure wires don't interfere with lid

5. **Test Before Closing**
   - Power on and verify functionality
   - Check serial output
   - Verify all connections

6. **Install Lid**
   - Position lid on top
   - Ensure lip fits inside box properly
   - If using screws, install 4× M3 × 10mm screws
   - Alternative: friction fit or use adhesive

7. **External Mounting (Optional)**
   - Use M3 × 20mm screws through bottom corner holes
   - Mount to wall, vehicle, or enclosure
   - Ensure ventilation slots are not blocked

## Customization Guide

### Adjust Dimensions

Edit these parameters in the `.scad` file:

```openscad
// Make it bigger
box_length = 120;  // Default: 100
box_width = 80;    // Default: 70
box_height = 50;   // Default: 40

// For different ESP32 boards
esp32_length = 55;  // Measure your board
esp32_width = 30;   // Measure your board

// Thicker walls for outdoor use
wall_thickness = 3.5;  // Default: 2.5

// Larger cable holes for thicker wires
cable_hole_diameter = 10;  // Default: 8
```

### Add Features

**Add mounting holes:**
```openscad
translate([x, y, -1])
    cylinder(d=3.2, h=wall_thickness+2, $fn=32);
```

**Add text label:**
```openscad
translate([x, y, wall_thickness/2])
    linear_extrude(height=wall_thickness/2 + 0.5)
    text("Your Text", size=5, halign="center");
```

**More ventilation:**
Change the loop counter:
```openscad
for(i = [0:9])  // Default: [0:5] for 6 slots
```

## Troubleshooting

### Print Issues

**Posts break off:**
- Increase wall thickness around posts
- Add more perimeters (4-5 instead of 3)
- Reduce print speed to 40mm/s
- Ensure supports are used

**Lid doesn't fit:**
- Increase `lid_gap` parameter (try 0.4 or 0.5)
- Check if printer is calibrated (dimensional accuracy)
- Sand the lip edges slightly

**Warping:**
- Use bed adhesion (brim or raft)
- Ensure bed is level
- Increase bed temperature by 5-10°C
- Reduce part cooling for first layers

**Layer adhesion poor:**
- Increase nozzle temperature by 5°C
- Slow down print speed
- Check filament for moisture

### Fit Issues

**ESP32 doesn't fit:**
- Measure your actual board dimensions
- Adjust `esp32_length` and `esp32_width` parameters
- Re-export and print

**Screw holes too tight:**
- Use 2.5mm or 3mm drill bit to clean holes
- Increase `mounting_hole_diameter` by 0.2mm
- Re-export and print

**Cables don't fit:**
- Increase `cable_hole_diameter`
- Use file or drill to enlarge existing holes
- Add additional cable holes in design

## Design Modifications

### Seal for Outdoor Use

Add gasket groove in lid:
```openscad
// In enclosure_lid() module, add:
translate([wall_thickness + 2, wall_thickness + 2, wall_thickness])
    difference() {
        rounded_box(inner_length - 4, inner_width - 4, 1.5, corner_radius-1);
        translate([1, 1, -1])
            rounded_box(inner_length - 6, inner_width - 6, 3, corner_radius-1.5);
    }
```

Use 2mm foam weather stripping as gasket.

### DIN Rail Mount

Add DIN rail clip to bottom:
```openscad
// See: https://www.thingiverse.com/thing:xxxx for clip design
// Or use commercial DIN rail adapters
```

### Wall Mount Flanges

Add mounting tabs to sides:
```openscad
// On sides of box
translate([0, box_width/2 - 10, box_height/4])
    cube([wall_thickness*2, 20, 2]);
```

## Alternative Enclosures

If 3D printing isn't available:

### Commercial Enclosures
- **Hammond 1591XXCSBK** (100×50×25mm, polycarbonate)
- **Bud Industries NBF-32026** (ABS project box)
- **Polycase WC-40** (IP65 rated)

Drill holes as needed for cables and mounting.

### DIY Alternatives
- Plastic food container (cheap, waterproof)
- Junction box from hardware store
- Mint tin (Altoids) for temporary/small builds
- 3D printing service (Shapeways, Sculpteo) if no printer

## Files & Downloads

### OpenSCAD File
`battery_monitor_enclosure.scad` - Fully parametric design

### Export Your Own STL
1. Open `.scad` file in OpenSCAD
2. Customize parameters
3. F6 to render
4. Export as STL

### Sharing
If you create improved versions:
- Fork the repository
- Submit pull request with your design
- Share on Thingiverse/Printables with link back

## License

This enclosure design is released under the same license as the main project (see main LICENSE file).

Free to use, modify, and distribute. Attribution appreciated!

## Credits

Design specifications based on:
- Standard ESP32 dev board dimensions
- Common battery monitor requirements
- 3D printing best practices

## Support

For issues with the enclosure design:
1. Check OpenSCAD syntax (F5 preview shows errors)
2. Verify dimensions match your components
3. Test print one part first
4. Open GitHub issue if problems persist

For 3D printing help:
- r/3Dprinting on Reddit
- OpenSCAD forums
- Your printer manufacturer support
