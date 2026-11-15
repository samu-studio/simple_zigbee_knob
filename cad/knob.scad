// Simple Zigbee Knob – parametric enclosure
// ESP32-C6 Super Mini, AS5600, 12x WS2812 ring (Ø50), 3x MX switch
//
// Export parts individually:  openscad -D 'part="shell"' -o shell.stl knob.scad
// part: assembly | shell | base | knob | diffuser | keycap | key_test | screw_test | mcu_test
//
// Coordinates: x = left/right (center 0), y = front(0) -> back(D), z = bottom(0) -> up
// The top surface is tilted by `tilt` (low at the front, high at the back). Everything that sits
// on the top (keys, knob, LED ring, AS5600) stands perpendicular to this slope.
// Local frame of the top (on_top): z = 0 is the outer surface, negative z points inward.

part = "assembly"; // [assembly, shell, base, knob, diffuser, keycap, key_test, screw_test, mcu_test]
$fn = 96;

/* ---------- Components (measure your parts!) ---------- */
knob_d        = 50;     // knob diameter
knob_h        = 20;
knob_grooves  = 36;     // grip grooves
knob_bevel    = 4;      // 45° chamfer at the bottom outer edge of the knob: reveals the diffuser

ring_od       = 50;     // LED ring outer diameter
ring_id       = 35;     // LED ring inner diameter
ring_t        = 3.3;    // PCB bottom to LED top
ring_pcb_t    = 1.6;    // ring PCB only (without LEDs), measured
ring_gap      = 5;      // air LED top -> diffuser (more = more even light, fewer hotspots)
ring_cable_w  = 5.5;    // width of the cable slot (radial, max. ~7)
ring_cable_arc= 50;     // length of the cable slot in degrees
ring_cable_dir= 270;    // direction of the slot (270 = front, 90 = back)

diffuser_t    = 1.5;    // print in transparent/white PETG
diffuser_d    = 60;     // diffuser outer diameter (larger than the knob = visible light rim)
diffuser_rim  = 1.2;    // outer support ledge, the light chamber is open below it
holder_d      = [34.0, 36.2]; // holder tube below the diffuser (inner, outer): presses on the inner edge of the ring PCB
holder_pre    = 0.2;    // preload of the holder tube on the PCB
diffuser_chamfer = 1.0; // 45° chamfer at the top outer edge (max. diffuser_t)
halo_gap      = 5;      // gap enclosure <-> knob bottom (the light comes out here)

shaft_d       = 12;     // knob shaft
hub_d         = 16;     // bearing surface on the enclosure
boss_len      = 10;     // length of the bearing boss
snap_lip      = 0.7;    // radial overhang of the snap lip
snap_lip_h    = 1.2;
finger_len    = 7;      // length of the snap fingers
finger_wall   = 1.6;
finger_slit   = 1.2;

magnet_d      = 4;      // DIAMETRICALLY magnetized!
magnet_h      = 2;
magnet_air_gap= 1.0;    // magnet -> chip surface (AS5600: 0.5 .. 3 mm)

as_pcb        = [23, 23];  // AS5600 module
as_pcb_t      = 1.6;
as_chip_h     = 0.9;
as_hole_pitch = 16;     // mounting holes on a square, center distance
as_standoff_d = 5.5;

mcu           = [18.0, 26]; // ESP32-C6 Super Mini (x = width, y = length)
mcu_pcb_t     = 1.1;
mcu_z         = 3;      // PCB bottom above the base plate
usb_w         = 9.4;    // USB-C opening
usb_h         = 3.6;
usb_center    = 1.6;    // USB center above the PCB top
usb_overhang  = 1.3;    // USB socket overhangs the PCB
// Mount in the base plate: one snap hook on each long side, a solid stop at the end opposite USB
// (takes the insertion force of the USB cable). At the USB end the back wall of the shell holds it.
// The hook nose has two 45° faces: the top one is the lead-in, the bottom one presses the PCB edge down and inward.
hook_ov       = 0.4;    // nose tip reaches this far over the PCB edge (edge pads start at 0.5 mm)
hook_gap      = 0.2;    // arm inner side -> PCB edge (the edge rests on the lower slope)
hook_pre      = 0.15;   // preload: the arm stays deflected by this much when snapped in
// The spring arm lies horizontally beside the PCB edge and flexes sideways: the bending stress
// runs along the print lines, not across the layer lines (vertical arms break there).
hook_t        = 2.0;    // arm thickness (flex direction)
hook_len      = 12;     // free arm length, from the nose center to the root
hook_w        = 10;     // length of the nose along the edge
hook_root     = 0.8;    // arm bottom above the plate bottom (relief cut in the plate)
side_hook_y   = 14;     // nose center of the side hooks, measured from the end opposite USB (root points to the front)
end_block_w   = 12;     // solid stop opposite USB: width
end_block_d   = [8, 3]; // depth bottom / top (wedge: the insertion force goes into the plate as pressure)
end_clr       = 0.1;    // gap PCB -> stop

key_count     = 3;
key_pitch     = 19.05;
key_hole      = 14.15;  // MX plate cutout (14.0 nominal + print shrinkage)
plate_t       = 1.5;    // MX clips need 1.5 mm
keycap_size   = 18;
key_gap       = 20;     // distance knob edge -> keycap edge

/* ---------- Screws (all M3) ---------- */
tap_d         = 2.75;   // core hole: M3 tapped directly into the plastic (found with the screw_test part)
screw_d       = 3.4;    // clearance hole
screw_head_d  = 6.4;    // countersunk head (base plate)
pan_head_d    = 5.6;    // pan/cheese head (AS5600, diffuser, clamp)
pan_head_h    = 2.4;
diff_screw_r  = 13.5;   // bolt circle of the diffuser screws (below the knob)

/* ---------- Enclosure ---------- */
tilt      = 8;      // tilt of the top surface in degrees (0 = flat)
H_front   = 18;     // height at the front edge
wall      = 2.4;
top_t     = 3;
corner_r  = 8;
margin    = 8;
top_margin = 5;     // margin between diffuser and enclosure edge
base_t    = 2.4;
clr       = 0.25;
boss_wall = 3;
screw_inset  = corner_r - 0.8;   // close to the corner radius -> countersink stays fully inside the base plate

/* ---------- Derived dimensions ---------- */
top_r = max(knob_d / 2 + margin, diffuser_d / 2 + top_margin);  // space needed around the knob center
W = max(2 * top_r, key_count * key_pitch + 2 * margin);
key_y  = margin + keycap_size / 2;
knob_y = margin + keycap_size + key_gap + knob_d / 2;
D = knob_y + top_r;
H_back = H_front + D * tan(tilt);
H_max  = H_back + 5;
function z_top(y) = H_front + y * tan(tilt);

pocket_r     = ring_od / 2 + 0.3;
pocket_ri    = min(ring_id / 2 - 0.5, holder_d[0] / 2 - 0.3);
diff_r       = diffuser_d / 2;
diff_ri      = hub_d / 2 + 1;
pocket_depth = ring_t + ring_gap + diffuser_t;
holder_len   = ring_gap + ring_t - ring_pcb_t + holder_pre;
cup_floor    = 1.2;
cup_r        = diff_r + 0.2 + 1.6;

// local z values below the top (at the knob)
cup_bottom   = -pocket_depth - cup_floor;
shaft_end_z  = -boss_len - 0.2 - snap_lip_h;
as_top_z     = shaft_end_z - magnet_air_gap - as_chip_h;
as_bot_z     = as_top_z - as_pcb_t;
core_d       = magnet_d + 2;                   // magnet carrier in the shaft center
finger_id    = shaft_d - 2 * finger_wall;

mcu_y1 = D - wall - usb_overhang - 0.2;   // back end of the PCB
mcu_y0 = mcu_y1 - mcu.y;
mcu_top = base_t + mcu_z + mcu_pcb_t;
usb_z  = mcu_top + usb_center;
hook_z0    = mcu_top - hook_pre - hook_gap;            // lower slope starts at the arm
hook_tip_z = hook_z0 + hook_gap + hook_ov;              // nose tip
hook_top   = hook_tip_z + hook_gap + hook_ov;

screw_pos = [for (sx = [-1, 1], y = [screw_inset, D - screw_inset]) [sx * (W / 2 - screw_inset), y]];
as_holes  = [for (sx = [-1, 1], sy = [-1, 1]) [sx * as_hole_pitch / 2, sy * as_hole_pitch / 2]];
diff_screws = [for (a = [0 : 90 : 270]) [diff_screw_r * cos(a), diff_screw_r * sin(a)]];

// global position of a point in the local frame at the knob
function knob_pt(yl, zl) = [knob_y + yl * cos(tilt) - zl * sin(tilt), z_top(knob_y) + yl * sin(tilt) + zl * cos(tilt)];
as_head_back = knob_pt(as_hole_pitch / 2, as_bot_z - pan_head_h);   // back AS5600 screw heads (above the Super Mini)
as_pcb_mid   = knob_pt(mcu_y0 - knob_y, as_bot_z);                   // AS5600 bottom above the stop
cup_back     = knob_pt(cup_r, cup_bottom);                           // lowest point of the ring cup at the back

echo(str("Enclosure: ", W, " x ", D, " mm, height front ", H_front, " / back ", H_back, " mm, tilt ", tilt, "°"));
echo(str("Clearance AS5600 screw heads -> Super Mini top: ", as_head_back[1] - mcu_top, " mm (wires on the pads need ~3 mm)"));
echo(str("Clearance AS5600 top -> ring cup: ", cup_bottom - as_top_z, " mm (components on the AS5600)"));
echo(str("Clearance AS5600 PCB -> stop opposite USB: ", as_pcb_mid[1] - (mcu_top + 1), " mm"));
echo(str("Ring cup back: ", cup_back[1] - mcu_top, " mm above Super Mini top, ", D - wall - (knob_y + cup_r * cos(tilt)), " mm in front of the back wall"));
echo(str("Clearance diffuser screw heads -> knob: ", halo_gap + 1.2 - pan_head_h, " mm"));

/* ---------- Helpers ---------- */
module rrect(w, d, h, r) {
  hull() for (x = [-w / 2 + r, w / 2 - r], y = [r, d - r])
    translate([x, y, 0]) cylinder(r = r, h = h);
}

module annulus(ro, ri, h) {
  difference() {
    cylinder(r = ro, h = h);
    translate([0, 0, -1]) cylinder(r = ri, h = h + 2);
  }
}

// local frame on the sloped top at depth y
module on_top(y) {
  translate([0, y, z_top(y)]) rotate([tilt, 0, 0]) children();
}

// everything below the top, offset inward by `off` (perpendicular to the slope)
module below_top(off = 0) {
  translate([0, 0, H_front]) rotate([tilt, 0, 0])
    translate([-W, -D, -off - 200]) cube([2 * W, 3 * D, 200]);
}

module outer_body() {
  intersection() {
    rrect(W, D, H_max, corner_r);
    below_top(0);
  }
}

module key_positions() {
  for (i = [0 : key_count - 1])
    on_top(key_y) translate([(i - (key_count - 1) / 2) * key_pitch, 0, 0]) children();
}

/* ---------- Top shell ---------- */
module shell() {
  difference() {
    intersection() {
      outer_body();
      union() {
        difference() {
          outer_body();
          intersection() {
            translate([0, wall, -1]) rrect(W - 2 * wall, D - 2 * wall, H_max + 2, corner_r - wall);
            below_top(top_t);
          }
        }
        on_top(knob_y) {
          // cup for the LED ring
          translate([0, 0, cup_bottom]) cylinder(r = cup_r, h = -cup_bottom + 0.5);
          // bearing boss
          translate([0, 0, -boss_len]) cylinder(d = shaft_d + 0.4 + 2 * boss_wall, h = boss_len + 0.5);
          // standoffs for the AS5600 (the PCB is screwed against them from below)
          for (p = as_holes) translate([p.x, p.y, as_top_z])
            cylinder(d = as_standoff_d, h = cup_bottom - as_top_z + 0.01);
        }
        // screw bosses (cut off at the top by the slope)
        // widened toward the corner so they stay connected to the wall
        for (p = screw_pos) hull() for (k = [0, 1])
          translate([p.x + k * sign(p.x) * 4, p.y + k * (p.y < D / 2 ? -4 : 4), base_t]) cylinder(d = 7, h = H_max);
      }
    }

    on_top(knob_y) {
      // ring pocket + diffuser recess
      translate([0, 0, -pocket_depth]) annulus(pocket_r, pocket_ri, pocket_depth + 1);
      translate([0, 0, -diffuser_t]) annulus(diff_r + 0.2, diff_ri - 0.2, diffuser_t + 1);
      // light chamber: widens from the ring to just inside the diffuser edge (support only at the outside)
      translate([0, 0, -pocket_depth + ring_t]) difference() {
        cylinder(r1 = pocket_r, r2 = diff_r - diffuser_rim, h = ring_gap + 0.01);
        translate([0, 0, -1]) cylinder(r = pocket_ri, h = ring_gap + 2);
      }

      // LED ring cable slot: slotted hole on the arc below the ring PCB
      translate([0, 0, cup_bottom - 1])
        for (i = [0 : 9]) hull() for (a = [i, i + 1])
          rotate(ring_cable_dir + (a / 10 - 0.5) * ring_cable_arc) translate([(ring_od + ring_id) / 4, 0, 0])
            cylinder(d = ring_cable_w, h = cup_floor + 2, $fn = 32);

      // shaft bore with chamfer
      translate([0, 0, -boss_len - 1]) cylinder(d = shaft_d + 0.4, h = boss_len + 2);
      translate([0, 0, -0.8]) cylinder(d1 = shaft_d + 0.4, d2 = shaft_d + 2, h = 0.81);
      // clearance below the boss for the snap fingers and the AS5600 chip
      translate([0, 0, as_top_z - 1]) cylinder(d = shaft_d + 2 * snap_lip + 2, h = -boss_len - as_top_z + 1);

      // core holes: diffuser (from above) and AS5600 (from below)
      for (p = diff_screws) translate([p.x, p.y, cup_bottom - 0.01]) cylinder(d = tap_d, h = -cup_bottom, $fn = 24);
      for (p = as_holes) translate([p.x, p.y, as_top_z - 1]) cylinder(d = tap_d, h = 1 + 8, $fn = 24);
    }

    // MX cutouts, relieved from below to 1.5 mm plate thickness
    key_positions() {
      translate([-key_hole / 2, -key_hole / 2, -top_t - 1]) cube([key_hole, key_hole, top_t + 2]);
      translate([-8, -8, -top_t - 1]) cube([16, 16, top_t - plate_t + 1]);
    }

    // USB-C
    hull() for (x = [-1, 1]) translate([x * (usb_w - usb_h) / 2, D + 1, usb_z])
      rotate([90, 0, 0]) cylinder(d = usb_h, h = wall + 2, $fn = 32);
    hull() for (x = [-1, 1]) translate([x * 2.5, D + 1, usb_z])
      rotate([90, 0, 0]) cylinder(d = 7.5, h = 2, $fn = 32);   // recess for the plug

    // core holes for the base plate
    for (p = screw_pos) translate([p.x, p.y, -1]) cylinder(d = tap_d, h = base_t + 13, $fn = 24);
  }
}

/* ---------- Base plate ---------- */
// Snap hook in the local frame: PCB edge at x = 0, PCB on the -x side.
module snap_hook() {
  y0 = -hook_w / 2;                  // free end
  y1 = hook_len + hook_w / 2;        // root
  x1 = hook_gap + hook_t;            // outer side of the arm
  // spring arm (free above the relief cut)
  translate([hook_gap, y0, hook_root]) cube([hook_t, y1 - y0, hook_top - hook_root]);
  // root, fixed to the plate, with a fillet to the arm
  translate([hook_gap, y1, 0]) cube([hook_t + 3, 4, hook_top]);
  translate([0, 0, hook_root]) linear_extrude(hook_top - hook_root)
    polygon([[x1 - 0.01, y1 - 2], [x1 - 0.01, y1 + 0.01], [x1 + 2, y1 + 0.01]]);
  // nose with 2x 45° at the free end: the bottom face presses the edge down/inward, the top is the lead-in
  translate([0, 0, 0]) rotate([90, 0, 0]) linear_extrude(hook_w, center = true) polygon([
    [hook_gap + 0.01, hook_z0], [-hook_ov, hook_tip_z], [hook_gap + 0.01, hook_top]]);
}
// relief cut below the arm, so it flexes freely up to the root
module snap_hook_relief() {
  translate([hook_gap - 0.6, -hook_w / 2 - 0.6, hook_root]) cube([hook_t + 2.6, hook_len + hook_w + 0.6, base_t]);
}
module mcu_hooks(relief = false) {
  for (sx = [-1, 1])
    translate([sx * mcu.x / 2, mcu_y0 + side_hook_y, 0]) mirror([sx > 0 ? 0 : 1, 0, 0]) mirror([0, 1, 0])
      if (relief) snap_hook_relief(); else snap_hook();
}
// supports, side hooks, solid stop opposite USB and small stops at the USB end
module mcu_mount() {
  // wedge opposite USB: the PCB end face rests against it, the insertion force goes diagonally into the plate
  translate([0, mcu_y0 - end_clr, base_t - 0.01]) rotate([90, 0, 90])
    linear_extrude(end_block_w, center = true) polygon([
      [0, 0], [-end_block_d[0], 0], [-end_block_d[1], mcu_top + 1 - base_t], [0, mcu_top + 1 - base_t]]);
  for (y = [mcu_y0 + 3, mcu_y1 - 3]) translate([-5, y - 1, base_t - 0.01]) cube([10, 2, mcu_z]);
  for (sx = [-1, 1]) translate([sx * (mcu.x / 2 - 1.5) - 1.5, mcu_y1 + 0.05, base_t - 0.01])
    cube([3, 1.1, mcu_z + mcu_pcb_t - 0.3]);   // stays below the USB socket
  mcu_hooks();
}

module base() {
  difference() {
    translate([0, wall + clr, 0])
      rrect(W - 2 * wall - 2 * clr, D - 2 * wall - 2 * clr, base_t, corner_r - wall - clr);
    mcu_hooks(relief = true);
    // countersunk screws
    for (p = screw_pos) translate([p.x, p.y, 0]) {
      translate([0, 0, -1]) cylinder(d = screw_d, h = base_t + 2, $fn = 24);
      translate([0, 0, -0.01]) cylinder(d1 = screw_head_d, d2 = screw_d, h = (screw_head_d - screw_d) / 2, $fn = 24);
    }
    // recesses for Ø10 rubber feet
    for (sx = [-1, 1], y = [14, D - 14]) translate([sx * (W / 2 - 14), y, -0.01]) cylinder(d = 10.5, h = 0.8);
  }
  mcu_mount();
}

/* ---------- Test part: Super Mini mount ---------- */
// Section of the base plate with supports, hooks and stops, to try out the clamping.
module mcu_test() {
  mx = mcu.x / 2 + hook_gap + hook_t + 7;
  my = max(end_block_d[0], hook_len + hook_w / 2 + 4 - side_hook_y) + 1;
  translate([0, -(mcu_y0 + mcu_y1) / 2, 0]) {
    difference() {
      translate([-mx, mcu_y0 - my, 0]) cube([2 * mx, mcu_y1 - mcu_y0 + my + 1.2, base_t]);
      mcu_hooks(relief = true);
      translate([0, (mcu_y0 + mcu_y1) / 2, base_t - 0.6]) linear_extrude(1)
        text(str(hook_ov, " / ", hook_pre), size = 2.6, halign = "center", valign = "center", font = "Liberation Sans:style=Bold");
    }
    mcu_mount();
  }
}

/* ---------- Knob ---------- */
module knob() {
  shaft_len = halo_gap + boss_len + 0.2 + snap_lip_h;
  difference() {
    union() {
      difference() {
        hull() {
          cylinder(d = knob_d, h = knob_h - 1.2);
          cylinder(d = knob_d - 2.4, h = knob_h);
        }
        for (i = [0 : knob_grooves - 1]) rotate(i * 360 / knob_grooves)
          translate([knob_d / 2 + 0.6, 0, 1.5]) cylinder(d = 2.4, h = knob_h, $fn = 16);
        // shallow recess at the bottom (reflects the light, room for the diffuser screw heads)
        translate([0, 0, -0.01]) annulus(knob_d / 2 - 2, hub_d / 2 + 2, 1.2);
        // chamfer at the bottom outer edge
        translate([0, 0, -0.01]) rotate_extrude() polygon([
          [knob_d / 2 - knob_bevel, 0], [knob_d / 2 + 1, 0], [knob_d / 2 + 1, knob_bevel + 1]]);
      }
      translate([0, 0, -halo_gap]) cylinder(d = hub_d, h = halo_gap + 0.01);
      translate([0, 0, -shaft_len]) cylinder(d = shaft_d, h = shaft_len + 0.01);
      // snap lip: sloped lead-in at the bottom, straight retaining edge at the top
      translate([0, 0, -shaft_len]) cylinder(d1 = shaft_d, d2 = shaft_d + 2 * snap_lip, h = snap_lip_h);
    }
    // ring groove: 4 spring fingers outside, a rigid core inside that carries the magnet.
    // This way the magnet does not bridge a slit and can be glued in.
    translate([0, 0, -shaft_len - 1]) annulus(finger_id / 2, core_d / 2, finger_len + 1);
    for (r = [0, 90]) rotate(r) difference() {
      translate([-shaft_d, -finger_slit / 2, -shaft_len - 1]) cube([2 * shaft_d, finger_slit, finger_len + 1]);
      translate([0, 0, -shaft_len - 2]) cylinder(d = core_d, h = finger_len + 3);
    }
    // magnet pocket (glue the magnet in)
    translate([0, 0, -shaft_len - 0.01]) cylinder(d = magnet_d + 0.15, h = magnet_h + 0.1, $fn = 32);
  }
}

/* ---------- Diffuser (4x M3x6 pan head) ---------- */
module diffuser() {
  difference() {
    // profile with 45° chamfer at the top outer edge (printed, the chamfer is at the bottom; 45° needs no supports)
    rotate_extrude($fn = 192) polygon([
      [diff_ri, 0], [diff_r, 0], [diff_r, diffuser_t - diffuser_chamfer],
      [diff_r - diffuser_chamfer, diffuser_t], [diff_ri, diffuser_t]]);
    for (p = diff_screws) translate([p.x, p.y, -1]) cylinder(d = screw_d, h = diffuser_t + 2, $fn = 24);
  }
  // holder tube: reaches from the diffuser bottom down to the ring PCB (between the LEDs and the inner edge)
  translate([0, 0, -holder_len]) annulus(holder_d[1] / 2, holder_d[0] / 2, holder_len + 0.01);
}

/* ---------- Simple MX keycap ---------- */
module keycap() {
  s = keycap_size; t = 13; h = 8; wk = 1.2;
  difference() {
    hull() {
      translate([0, 0, 0.5]) cube([s, s, 1], center = true);
      translate([0, 0, h - 0.5]) cube([t, t, 1], center = true);
    }
    hull() {
      translate([0, 0, -0.5]) cube([s - 2 * wk, s - 2 * wk, 1], center = true);
      translate([0, 0, h - wk - 0.5]) cube([t - 2 * wk, t - 2 * wk, 1], center = true);
    }
  }
  difference() {
    translate([0, 0, 1]) cylinder(d = 5.5, h = h - wk - 0.99, $fn = 32);
    for (r = [0, 90]) rotate(r) translate([-4.15 / 2, -1.35 / 2, 0.9]) cube([4.15, 1.35, 4.0]);
  }
}

/* ---------- Test part: one MX cutout like in the shell ---------- */
// Same wall thickness and relief as the shell, printed top side down.
module key_test() {
  s = key_pitch + 6;
  difference() {
    translate([-s / 2, -s / 2, 0]) cube([s, s, top_t]);
    translate([-key_hole / 2, -key_hole / 2, -1]) cube([key_hole, key_hole, top_t + 2]);
    translate([-8, -8, plate_t]) cube([16, 16, top_t]);
    // dimension label on the bottom
    translate([0, -s / 2 + 1.6, top_t - 0.6]) linear_extrude(1)
      text(str(key_hole), size = 2.6, halign = "center", font = "Liberation Sans:style=Bold");
  }
}

/* ---------- Test part: core holes 2.70 … 3.10 mm ---------- */
// Posts like the screw bosses (Ø7, printed vertically, same polygon count as in the enclosure),
// so the best size can be used directly as tap_d.
screw_test_d = [for (i = [0 : 8]) 2.70 + 0.05 * i];
module screw_test() {
  pitch = 13; plate = 2; post_h = 10;
  for (i = [0 : len(screw_test_d) - 1]) {
    x = (i % 3 - 1) * pitch;
    y = (1 - floor(i / 3)) * (pitch + 3);
    d = screw_test_d[i];
    translate([x, y, 0]) difference() {
      union() {
        translate([-pitch / 2, -(pitch + 3) / 2, 0]) cube([pitch, pitch + 3, plate]);
        translate([0, 1.5, 0]) cylinder(d = 7, h = post_h);
      }
      translate([0, 1.5, 0.6]) cylinder(d = d, h = post_h, $fn = 24);
      translate([0, -(pitch + 3) / 2 + 1.2, plate - 0.6]) linear_extrude(1)
        text(let (c = round(d * 100)) str(floor(c / 100), ".", c % 100 < 10 ? "0" : "", c % 100), size = 2.6, halign = "center", font = "Liberation Sans:style=Bold");
    }
  }
}

/* ---------- Output ---------- */
// Shell: sloped top flat on the print bed
if (part == "shell")     rotate([180, 0, 0]) rotate([-tilt, 0, 0]) translate([0, 0, -H_front]) shell();
if (part == "base")      base();
if (part == "knob")      rotate([180, 0, 0]) translate([0, 0, -knob_h]) knob();
if (part == "diffuser")  rotate([180, 0, 0]) translate([0, 0, -diffuser_t]) diffuser();   // visible side on the bed, holder tube up
if (part == "keycap")    rotate([180, 0, 0]) translate([0, 0, -8]) keycap();
if (part == "key_test")  key_test();
if (part == "screw_test") screw_test();
if (part == "mcu_test")  mcu_test();

if (part == "assembly") {
  color("#3a3d42") shell();
  color("#2b2d31") base();
  on_top(knob_y) {
    color("white", 0.6) translate([0, 0, -diffuser_t]) diffuser();
    color("#1f1f22") translate([0, 0, halo_gap]) knob();
    // placeholders
    %translate([0, 0, -pocket_depth]) annulus(ring_od / 2, ring_id / 2, ring_t);
    %translate([-as_pcb.x / 2, -as_pcb.y / 2, as_bot_z]) cube([as_pcb.x, as_pcb.y, as_pcb_t]);
  }
  color("#d9d9d9") key_positions() translate([0, 0, 7]) keycap();
  %translate([-mcu.x / 2, mcu_y0, base_t + mcu_z]) cube([mcu.x, mcu.y, mcu_pcb_t]);
}
