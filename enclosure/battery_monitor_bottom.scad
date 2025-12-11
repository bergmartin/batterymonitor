/*
 * ESP32 Battery Monitor Enclosure - BOTTOM PART
 * 
 * OpenSCAD script to generate 3D printable enclosure bottom
 * 
 * To use:
 * 1. Install OpenSCAD (free): https://openscad.org/
 * 2. Open this file in OpenSCAD
 * 3. Press F5 to preview
 * 4. Press F6 to render
 * 5. File > Export > Export as STL
 * 
 * Print settings:
 * - Layer height: 0.2mm
 * - Infill: 20%
 * - Supports: Yes (for mounting posts)
 * - Material: PLA or PETG
 */

// ============================================================================
// Configuration Parameters (adjust as needed)
// ============================================================================

// Box dimensions (in mm)
box_length = 100;      // X dimension
box_width = 70;        // Y dimension
box_height = 40;       // Z dimension (total height)
wall_thickness = 2.5;  // Wall thickness

// ESP32 board dimensions (typical dev board)
esp32_length = 51;     // PCM board length
esp32_width = 28;      // PCM board width
esp32_height = 15;     // Including USB connector

// Component spacing
bottom_clearance = 3;  // Space under ESP32 for wiring
top_clearance = 10;    // Space above ESP32

// Mounting
mounting_hole_diameter = 3.2;  // M3 screw holes
mounting_post_diameter = 6;    // Posts for ESP32
mounting_post_height = 5;      // Height of mounting posts

// Cable management
cable_hole_diameter = 8;  // For wires entering box

// Rounded corners
corner_radius = 3;

// ============================================================================
// Calculated Values
// ============================================================================

inner_length = box_length - 2 * wall_thickness;
inner_width = box_width - 2 * wall_thickness;
inner_height = box_height - wall_thickness;

// ============================================================================
// Modules
// ============================================================================

// Rounded box module
module rounded_box(length, width, height, radius) {
    hull() {
        translate([radius, radius, 0])
            cylinder(r=radius, h=height, $fn=32);
        translate([length-radius, radius, 0])
            cylinder(r=radius, h=height, $fn=32);
        translate([radius, width-radius, 0])
            cylinder(r=radius, h=height, $fn=32);
        translate([length-radius, width-radius, 0])
            cylinder(r=radius, h=height, $fn=32);
    }
}

// Main box (bottom part)
module enclosure_bottom() {
    difference() {
        // Outer shell
        rounded_box(box_length, box_width, box_height/2 + wall_thickness, corner_radius);
        
        // Inner cavity
        translate([wall_thickness, wall_thickness, wall_thickness])
            rounded_box(inner_length, inner_width, box_height/2 + 1, corner_radius-0.5);
        
        // Cable entry holes
        // Bottom cable hole (for power)
        translate([box_length/2, wall_thickness/2, wall_thickness + 5])
            rotate([-90, 0, 0])
            cylinder(d=cable_hole_diameter, h=wall_thickness*2, $fn=32);
        
        // Side cable hole (optional for voltage divider)
        translate([wall_thickness/2, box_width/2, wall_thickness + 5])
            rotate([0, 90, 0])
            cylinder(d=cable_hole_diameter, h=wall_thickness*2, $fn=32);
        
        // Mounting holes in bottom (for external mounting)
        for(x = [10, box_length-10]) {
            for(y = [10, box_width-10]) {
                translate([x, y, -1])
                    cylinder(d=mounting_hole_diameter, h=wall_thickness+2, $fn=32);
            }
        }
        
        // Label area (recessed text placeholder)
        translate([box_length/2, box_width - 8, wall_thickness/2])
            linear_extrude(height=wall_thickness/2 + 0.5)
            text("Battery Monitor", size=5, halign="center", valign="center", font="Liberation Sans:style=Bold");
    }
    
    // ESP32 mounting posts
    post_offset_x = (box_length - esp32_length) / 2;
    post_offset_y = (box_width - esp32_width) / 2;
    
    for(x = [post_offset_x, post_offset_x + esp32_length - mounting_post_diameter]) {
        for(y = [post_offset_y, post_offset_y + esp32_width - mounting_post_diameter]) {
            translate([x + mounting_post_diameter/2, y + mounting_post_diameter/2, wall_thickness]) {
                difference() {
                    cylinder(d=mounting_post_diameter, h=mounting_post_height, $fn=32);
                    // Screw hole
                    translate([0, 0, -1])
                        cylinder(d=2.5, h=mounting_post_height+2, $fn=16);
                }
            }
        }
    }
    
    // Wire management clips
    translate([wall_thickness + 5, wall_thickness + 2, wall_thickness])
        cube([3, 2, 4]);
    translate([box_length - wall_thickness - 8, wall_thickness + 2, wall_thickness])
        cube([3, 2, 4]);
}

// ============================================================================
// Generate Bottom Part
// ============================================================================

enclosure_bottom();

// ============================================================================
// Print Instructions
// ============================================================================

/*
TO EXPORT STL FILE:
1. F6 to render (may take a few seconds)
2. File > Export > Export as STL
3. Save as: battery_monitor_bottom.stl

3D PRINTING SETTINGS:
- Layer height: 0.2mm
- Shell thickness: 3 perimeters (1.2mm)
- Infill: 20%
- Supports: Yes (for mounting posts)
- Bed adhesion: Brim recommended
- Material: PLA or PETG
- Print time: ~1.5-2 hours

POST-PROCESSING:
- Remove supports carefully from mounting posts
- Clean mounting post holes with 2.5mm drill bit if needed
- Test fit ESP32 board before final assembly
- Verify cable holes are clear and smooth

ASSEMBLY:
1. Insert ESP32 board into bottom part
2. Align mounting holes with posts
3. Secure with M3 x 6mm screws from bottom
4. Route wiring through cable holes
5. Test functionality before adding lid

CUSTOMIZATION:
Adjust parameters at top of file:
- box_length, box_width, box_height: Overall dimensions
- esp32_length, esp32_width: Match your board
- cable_hole_diameter: For your wire gauge
- mounting_post_height: For different ESP32 heights
*/
