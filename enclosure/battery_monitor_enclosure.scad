/*
 * ESP32 Battery Monitor Enclosure
 * 
 * OpenSCAD script to generate 3D printable enclosure
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
 * - Supports: Yes (for mounting holes)
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

// Lid
lid_lip_height = 3;    // Overlap between lid and box
lid_gap = 0.3;         // Clearance gap

// Ventilation
vent_slot_width = 2;
vent_slot_length = 20;
vent_slot_spacing = 4;

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

// Lid (top part)
module enclosure_lid() {
    difference() {
        union() {
            // Main lid
            rounded_box(box_length, box_width, wall_thickness, corner_radius);
            
            // Lip that fits inside box
            translate([wall_thickness + lid_gap, wall_thickness + lid_gap, wall_thickness])
                rounded_box(
                    inner_length - 2*lid_gap, 
                    inner_width - 2*lid_gap, 
                    lid_lip_height, 
                    corner_radius-1
                );
        }
        
        // Ventilation slots
        for(i = [0:5]) {
            translate([box_length/2 - 3*vent_slot_spacing + i*vent_slot_spacing, 
                      box_width/2 - vent_slot_length/2, 
                      -1])
                cube([vent_slot_width, vent_slot_length, wall_thickness+2]);
        }
        
        // LED indicator hole (if you add one)
        translate([box_length - 10, box_width - 10, -1])
            cylinder(d=5, h=wall_thickness+2, $fn=32);
        
        // Mounting screw holes for lid
        for(x = [10, box_length-10]) {
            for(y = [10, box_width-10]) {
                translate([x, y, -1])
                    cylinder(d=mounting_hole_diameter, h=wall_thickness+2, $fn=32);
                // Countersink
                translate([x, y, wall_thickness/2])
                    cylinder(d1=mounting_hole_diameter, d2=mounting_hole_diameter+2, h=wall_thickness/2+1, $fn=32);
            }
        }
    }
}

// ============================================================================
// Assembly View (comment out parts for individual printing)
// ============================================================================

// Print bottom part
enclosure_bottom();

// Print lid separately (move it for viewing)
// Uncomment to see lid positioned above box:
// translate([0, 0, box_height/2 + wall_thickness + 5])
//     enclosure_lid();

// Or export them separately for printing:
// For bottom: keep only enclosure_bottom() uncommented
// For lid: comment out bottom, uncomment lid only:
// enclosure_lid();

// ============================================================================
// Print Instructions
// ============================================================================

/*
TO EXPORT STL FILES:

1. BOTTOM PART:
   - Keep only "enclosure_bottom();" uncommented
   - F6 to render
   - File > Export > Export as STL
   - Save as: battery_monitor_bottom.stl

2. LID PART:
   - Comment out "enclosure_bottom();"
   - Uncomment only "enclosure_lid();"
   - F6 to render
   - File > Export > Export as STL
   - Save as: battery_monitor_lid.stl

3D PRINTING SETTINGS:
- Layer height: 0.2mm
- Shell thickness: 3 perimeters (1.2mm)
- Infill: 20%
- Supports: Yes (for mounting posts)
- Bed adhesion: Brim recommended
- Material: PLA or PETG
- Print time: ~2-3 hours total

POST-PROCESSING:
- Remove supports carefully
- Clean mounting post holes with 2.5mm drill bit if needed
- Test fit ESP32 board before final assembly
- Use M3 screws to mount ESP32 to posts
- Use M3 screws to attach lid to box

ASSEMBLY:
1. Mount ESP32 to bottom using M3 x 6mm screws
2. Connect wiring through cable holes
3. Test functionality before closing
4. Place lid on top
5. Secure with M3 x 10mm screws (optional)

CUSTOMIZATION:
Adjust parameters at top of file:
- box_length, box_width, box_height: Overall dimensions
- esp32_length, esp32_width: Match your board
- cable_hole_diameter: For your wire gauge
- Add more features as needed
*/
