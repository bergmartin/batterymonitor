/*
 * ESP32 Battery Monitor Enclosure - LID PART
 * 
 * OpenSCAD script to generate 3D printable enclosure lid
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
 * - Supports: No
 * - Material: PLA or PETG
 */

// ============================================================================
// Configuration Parameters (adjust as needed)
// ============================================================================

// Box dimensions (in mm) - MUST MATCH BOTTOM PART
box_length = 100;      // X dimension
box_width = 70;        // Y dimension
box_height = 40;       // Z dimension (total height)
wall_thickness = 2.5;  // Wall thickness

// Lid
lid_lip_height = 3;    // Overlap between lid and box
lid_gap = 0.3;         // Clearance gap

// Ventilation
vent_slot_width = 2;
vent_slot_length = 20;
vent_slot_spacing = 4;

// Mounting
mounting_hole_diameter = 3.2;  // M3 screw holes

// Rounded corners
corner_radius = 3;

// ============================================================================
// Calculated Values
// ============================================================================

inner_length = box_length - 2 * wall_thickness;
inner_width = box_width - 2 * wall_thickness;

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
// Generate Lid Part
// ============================================================================

enclosure_lid();

// ============================================================================
// Print Instructions
// ============================================================================

/*
TO EXPORT STL FILE:
1. F6 to render (may take a few seconds)
2. File > Export > Export as STL
3. Save as: battery_monitor_lid.stl

3D PRINTING SETTINGS:
- Layer height: 0.2mm
- Shell thickness: 3 perimeters (1.2mm)
- Infill: 20%
- Supports: No (prints flat)
- Bed adhesion: Brim optional
- Material: PLA or PETG (match bottom part)
- Print time: ~1-1.5 hours

POST-PROCESSING:
- Remove any brim or support material
- Clean mounting screw holes if needed
- Verify ventilation slots are clear
- Test fit on bottom part before assembly

ASSEMBLY:
1. Ensure all components are installed in bottom part
2. Test functionality with lid open
3. Place lid on top of bottom part
4. Verify lip fits inside bottom part
5. Secure with M3 x 10mm screws through mounting holes (optional)
6. For permanent seal, can use small amount of adhesive

CUSTOMIZATION:
Adjust parameters at top of file:
- IMPORTANT: Dimensions must match bottom part!
- lid_gap: Increase for looser fit, decrease for tighter fit
- vent_slot_width/length: Adjust ventilation
- lid_lip_height: Change snap-fit depth
*/
