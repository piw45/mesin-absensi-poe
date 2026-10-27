// ============================================================================
// 3D PRINT ENCLOSURE - MESIN ABSENSI SIAKAD DENANYAR (OPSI 2: LAN8720A RMII)
// SINKRON 100% DENGAN PCB PORTRAIT 80.0 x 100.0 mm (absensi_poe.kicad_pcb)
// ----------------------------------------------------------------------------
// Dimensi Luar Box : 88.0 mm (Lebar X) x 108.0 mm (Tinggi Y) x 38.0 mm (Tebal Z)
// Tebal Dinding    : 2.5 mm (Rongga Dalam: 83.0 x 103.0 mm -> Clearance 1.5mm/sisi)
// Jarak Standoff M3: 72.0 mm (X) x 92.0 mm (Y) -> Tepat di (8,8), (80,8), (8,100), (80,100)
// Koordinat Offset : X_box = X_pcb + 4.0 mm,  Y_box = (100.0 - Y_pcb) + 4.0 mm
// ============================================================================

$fn = 64;

box_w  = 88.0;   // Lebar luar X (PCB 80mm + 2*1.5mm clearance + 2*2.5mm wall)
box_l  = 108.0;  // Panjang luar Y (PCB 100mm + 2*1.5mm clearance + 2*2.5mm wall)
body_h = 33.0;   // Tinggi bodi bawah Z
lid_h  = 5.0;    // Tinggi tutup atas Z (Total tinggi rakitan = 38.0 mm)
wall   = 2.5;    // Tebal dinding utama
corner_r = 5.0;  // Radius sudut luar

module rounded_block(w, l, h, r) {
    hull() {
        for (x = [r, w - r])
            for (y = [r, l - r])
                translate([x, y, 0]) cylinder(r = r, h = h);
    }
}

// BAGIAN 1: BODI BAWAH
module box_body() {
    difference() {
        rounded_block(box_w, box_l, body_h, corner_r);

        translate([wall, wall, wall])
            rounded_block(box_w - 2*wall, box_l - 2*wall, body_h, corner_r - wall);

        translate([wall - 0.8, wall - 0.8, body_h - 3.0])
            rounded_block(box_w - 2*(wall - 0.8), box_l - 2*(wall - 0.8), 3.5, corner_r - wall + 0.8);

        // Cutout SATU-SATUNYA Port RJ45 PoE di Dinding Bawah (X_center = 44.0 mm)
        translate([44.0 - 8.4, -1.0, wall + 6.0])
            cube([16.8, wall + 5.0, 14.5]);

        // 2x Lubang Keyhole Gantungan Dinding Baut M4 di Belakang
        for (kx = [24.0, 64.0]) {
            translate([kx, 86.0, -0.5]) cylinder(d = 8.6, h = wall + 1.5);
            translate([kx - 2.25, 86.0, -0.5]) cube([4.5, 9.0, wall + 1.5]);
            translate([kx, 86.0, wall - 0.8]) cylinder(d = 10.0, h = 1.5);
            translate([kx - 4.3, 86.0, wall - 0.8]) cube([8.6, 9.0, 1.5]);
        }

        // Ventilasi samping
        for (vy = [32.0 : 6.0 : 62.0]) {
            translate([-1.0, vy, 14.0]) cube([wall + 2.0, 2.5, 12.0]);
            translate([box_w - wall - 1.0, vy, 14.0]) cube([wall + 2.0, 2.5, 12.0]);
        }
    }

    // 4x Tiang Standoff PCB M3 (72.0 x 92.0 mm)
    for (sx = [8.0, 80.0]) {
        for (sy = [8.0, 100.0]) {
            translate([sx, sy, wall]) {
                difference() {
                    union() {
                        cylinder(d1 = 9.0, d2 = 7.2, h = 6.0);
                        translate([-1.2, -4.0, 0]) cube([2.4, 8.0, 4.0]);
                        translate([-4.0, -1.2, 0]) cube([8.0, 2.4, 4.0]);
                    }
                    translate([0, 0, 0.5]) cylinder(d = 2.8, h = 6.0);
                }
            }
        }
    }

    // Dudukan dasar sensor AS608
    translate([44.0 - 12.0, 31.0 - 11.0, wall]) {
        difference() {
            cube([24.0, 22.0, 8.5]);
            translate([2.0, 2.0, -0.5]) cube([20.0, 18.0, 10.0]);
        }
    }
}

// BAGIAN 2: TUTUP ATAS / FRONT PANEL
module box_lid() {
    difference() {
        union() {
            rounded_block(box_w, box_l, lid_h, corner_r);

            translate([wall - 0.5, wall - 0.5, -2.5])
                rounded_block(box_w - 2*(wall - 0.5), box_l - 2*(wall - 0.5), 2.5, corner_r - wall + 0.5);

            translate([44.0 - 12.5, 31.0 - 11.0, -14.0])
                cube([25.0, 22.0, 14.0]);
        }

        translate([wall + 1.5, wall + 1.5, -3.0])
            rounded_block(box_w - 2*(wall + 1.5), box_l - 2*(wall + 1.5), 3.5, corner_r - wall - 1.5);

        // A. JENDELA LAYAR OLED 0.91" (128x32)
        translate([44.0 - 13.5, 93.5 - 5.25, -3.5])
            cube([27.0, 10.5, lid_h + 4.5]);
        hull() {
            translate([44.0 - 13.5, 93.5 - 5.25, lid_h - 1.8]) cube([27.0, 10.5, 0.1]);
            translate([44.0 - 16.0, 93.5 - 7.5,  lid_h + 0.1]) cube([32.0, 15.0, 0.1]);
        }

        // B. SLOT SENSOR SIDIK JARI AS608
        translate([44.0 - 10.6, 31.0 - 9.5, -15.0])
            cube([21.2, 19.0, lid_h + 16.0]);
        translate([44.0 - 11.2, 31.0 - 10.0, -15.0])
            cube([22.4, 20.0, 13.5]);
        hull() {
            translate([44.0 - 10.6, 31.0 - 9.5, lid_h - 2.2]) cube([21.2, 19.0, 0.1]);
            translate([44.0 - 15.0, 31.0 - 14.0, lid_h + 0.1]) cube([30.0, 28.0, 0.1]);
        }

        // C. GRILL AKUSTIK BUZZER 12mm
        for (i = [-1.5, -0.5, 0.5, 1.5]) {
            translate([20.0 - 5.5, 20.0 + i * 3.2 - 0.8, -3.5])
                cube([11.0, 1.6, lid_h + 5.0]);
        }

        // D. Frame Dekoratif
        difference() {
            translate([8.0, 8.0, lid_h - 0.6]) rounded_block(box_w - 16.0, box_l - 16.0, 1.0, 3.0);
            translate([9.2, 9.2, lid_h - 0.8]) rounded_block(box_w - 18.4, box_l - 18.4, 1.5, 2.0);
        }
    }
}

box_body();
translate([box_w + 18.0, 0, 2.5]) box_lid();
