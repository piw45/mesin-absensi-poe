// ============================================================================
// 3D PRINT ENCLOSURE - VARIAN 1: W5500 BIRU BESAR (HR911105A) + PERFBOARD 4x6 CM
// MODEL PORTRAIT ALA ZKTECO (SESUAI SKETSA IMG_7761.HEIC):
//   - OLED 0.91" di TENGAH-ATAS panel depan
//   - AS608 posisi REBAHAN HORIZONTAL (memanjang 56mm ke dalam arah Z!) di ceruk
//     bawah-tengah (kantong jari 28 x 44.5 mm, kedalaman jari 26 mm, baffle rapi)
//   - Mainboard 40x60 + W5500 Biru di separuh ATAS bodi belakang OLED (nol tabrakan)
// Mesin Absensi SIAKAD Denanyar (Unit A & Unit B dari IMG_7760.MOV)
// ----------------------------------------------------------------------------
// Dimensi Luar Box  : 120.0 (X) x 142.0 (Y) x 68.0 (Z) mm (Body 63.5 + Lid 4.5)
// Export STL        : Set PART="body" atau PART="lid" (lid otomatis terbalik Z=0)
// ============================================================================

// Pilih mode render/export: "preview" | "assembled" | "body" | "lid"
PART = "preview";

// Mode bukaan USB-C di dinding atas (Y=box_l):
//   "universal" : slot vertikal 23.0 mm (muat Unit A ESP32 atas & Unit B ESP32 bawah)
//   "top"       : slot 10.5 mm khusus Unit A (ESP32 di atas perfboard)
//   "bottom"    : slot 10.5 mm khusus Unit B (ESP32 di bawah perfboard)
USB_MOUNT_MODE = "universal";

// Jarak renggang Z untuk visualisasi PART="assembled" (0 = tertutup rapat)
ASSEMBLE_GAP = 0.0;

$fn = 64;

// --- PARAMETER DIMENSI UTAMA (mm) ---
box_w    = 120.0;  // Lebar luar X (perfboard 60 + W5500 menjorok kiri 45 + margin)
box_l    = 142.0;  // Tinggi luar Y (ceruk AS608 bawah 49mm + mainboard atas)
body_h   = 63.5;   // Kedalaman bodi Z (muat AS608 rebahan 56mm + ruang kabel JST belakang)
lid_h    = 4.5;    // Tebal tutup depan Z (Total kedalaman = 68.0 mm)
wall     = 2.5;    // Tebal dinding utama
corner_r = 5.0;    // Radius sudut luar

// Posisi lubang baut M3 sudut (4 titik)
boss_pts = [
    [8.0, 8.0],
    [box_w - 8.0, 8.0],
    [8.0, box_l - 8.0],
    [box_w - 8.0, box_l - 8.0]
];

// Parameter port dinding bodi (sesuai orientasi 0 deg di atas)
rj45_cy  = 109.0;  // Port RJ45 W5500 Biru di dinding KIRI (X=0)
rj45_w   = 20.0;
rj45_h   = 17.5;
rj45_z0  = wall + 14.5;

usbc_cx  = 91.0;   // Port USB-C ESP32 di dinding ATAS (Y=box_l)
usbc_w   = 14.0;

// Parameter panel depan simetris (X = 60.0 mm)
as608_cx = 60.0;   // Pusat terowongan/ceruk jari AS608 di bawah tengah
oled_cx  = 60.0;   // Pusat OLED 0.91" di tengah atas
oled_cy  = 96.0;
buzz_cx  = 60.0;   // Pusat grill akustik buzzer
buzz_cy  = 66.0;

// --- MODUL BANTU GEOMETRI ---
module rounded_block(w, l, h, r) {
    hull() {
        for (x = [r, w - r])
            for (y = [r, l - r])
                translate([x, y, 0]) cylinder(r = r, h = h);
    }
}

module usbc_cutout() {
    z_bot = (USB_MOUNT_MODE == "top") ? (wall + 15.5) :
            (USB_MOUNT_MODE == "bottom") ? (wall + 3.5) : (wall + 4.0);
    slot_h = (USB_MOUNT_MODE == "universal") ? 23.0 : 10.5;
    r_slot = 2.0;

    // Slot berujung bulat pada dinding ATAS (Y=box_l) sepanjang sumbu Y
    hull() {
        for (dx = [-(usbc_w / 2 - r_slot), (usbc_w / 2 - r_slot)]) {
            for (dz = [r_slot, slot_h - r_slot]) {
                translate([usbc_cx + dx, box_l - wall - 1.0, z_bot + dz])
                    rotate([-90, 0, 0])
                    cylinder(r = r_slot, h = wall + 2.5);
            }
        }
    }
}

// Terowongan/Ceruk Jari AS608 Rebahan Arah Z (kedalaman ~56mm tegak lurus dinding)
module as608_tunnel(cx) {
    difference() {
        union() {
            // Dinding Kiri & Kanan terowongan (rongga dalam lebar 28.0 mm: cx-14..cx+14)
            translate([cx - 16.0, wall - 0.2, wall - 0.2])
                cube([2.0, 49.0 - wall + 0.2, body_h - wall + 0.2]);
            translate([cx + 14.0, wall - 0.2, wall - 0.2])
                cube([2.0, 49.0 - wall + 0.2, body_h - wall + 0.2]);

            // Atap terowongan (Y=47..49, menutup tinggi ceruk 44.5mm)
            translate([cx - 16.0, 47.0, wall - 0.2])
                cube([32.0, 2.0, body_h - wall + 0.2]);

            // Rel pemandu tengah AS608 di bagian bawah (sisakan lebar pas 24.4mm untuk bodi 23.5mm)
            translate([cx - 14.2, wall - 0.2, 12.0])
                cube([2.0, 18.5 - wall + 0.2, body_h - 12.0]);
            translate([cx + 12.2, wall - 0.2, 12.0])
                cube([2.0, 18.5 - wall + 0.2, body_h - 12.0]);

            // Plafon penjepit bodi kamera belakang AS608 (Y=25..27, bebas overhang dari lantai Z=2.5)
            translate([cx - 14.2, 25.0, wall - 0.2])
                cube([28.4, 2.0, 42.0 - wall + 0.2]);

            // Baffle penutup depan atas (Z=40..42, Y=26.5..47.5) dengan lereng 45 deg agar cetak 0 support
            hull() {
                translate([cx - 14.2, 26.5, 40.0])
                    cube([28.4, 21.0, 2.0]);
                translate([cx - 14.2, 46.5, 19.5])
                    cube([28.4, 1.0, 2.0]);
            }

            // 2x Tiang stopper belakang AS608 pada Z=12.0 (tengah terbuka 18mm untuk konektor JST 6-pin)
            translate([cx - 14.2, wall - 0.2, wall - 0.2])
                cube([5.2, 8.5 - wall + 0.2, 12.0 - wall + 0.2]);
            translate([cx + 9.0, wall - 0.2, wall - 0.2])
                cube([5.2, 8.5 - wall + 0.2, 12.0 - wall + 0.2]);
        }

        // Portal keluar kabel JST 6-pin di kiri & kanan zona belakang (Z=2.2..16.5) menuju mainboard
        translate([cx - 17.0, wall - 0.3, wall - 0.3])
            cube([6.0, 18.8 - wall, 16.5 - wall + 0.3]);
        translate([cx + 11.0, wall - 0.3, wall - 0.3])
            cube([6.0, 18.8 - wall, 16.5 - wall + 0.3]);
    }
}

// --- BAGIAN 1: BODI BAWAH ---
module box_body() {
    difference() {
        union() {
            // Dinding luar + lantai belakang berongga
            difference() {
                rounded_block(box_w, box_l, body_h, corner_r);

                // Rongga dalam (X=2.5..117.5, Y=2.5..139.5, Z=2.5..63.5)
                translate([wall, wall, wall])
                    rounded_block(box_w - 2*wall, box_l - 2*wall, body_h + 1.0, corner_r - wall);

                // Step rabbet bibir tutup (kedalaman 2.5mm, mundur 0.9mm ke dinding)
                translate([wall - 0.9, wall - 0.9, body_h - 2.5])
                    rounded_block(box_w - 2*(wall - 0.9), box_l - 2*(wall - 0.9), 3.5, corner_r - wall + 0.9);
            }

            // 4x Boss Sekrup M3 Sudut + Rusuk Penguat
            for (p = boss_pts) {
                sx = p[0];
                sy = p[1];
                dx = (sx < box_w / 2) ? -1 : 1;
                dy = (sy < box_l / 2) ? -1 : 1;
                translate([sx, sy, wall - 0.2]) {
                    cylinder(d = 8.0, h = body_h - wall + 0.2);
                    translate([dx > 0 ? 0 : -5.5, -1.2, 0])
                        cube([5.5, 2.4, body_h - wall - 2.5]);
                    translate([-1.2, dy > 0 ? 0 : -5.5, 0])
                        cube([2.4, 5.5, body_h - wall - 2.5]);
                }
            }

            // Ceruk Terowongan AS608 Rebahan di Bawah-Tengah (X=60.0)
            as608_tunnel(as608_cx);

            // Bay Mainboard Atas (Perfboard 60x40 pada X=50..110, Y=89..129, Z_in=14.0mm)
            // Zona X=75..107 dibiarkan terbuka di bawah untuk ESP32 posisi bawah [Unit B]

            // Rak Kiri-Bawah + Backstop Dorongan Kabel USB-C
            translate([50.0, 86.0, wall - 0.2])
                cube([23.0, 8.0, 14.2]);
            translate([50.0, 86.0, wall + 13.8])
                cube([23.0, 3.0, 8.2]);

            // Rak Kiri-Atas + Bibir Penahan Atas (saling tumpang tindih 0.5mm di Y=129.0..129.5)
            translate([50.0, 123.5, wall - 0.2])
                cube([23.0, 6.0, 14.2]);
            translate([50.0, 129.0, wall - 0.2])
                cube([23.0, 2.5, 16.2]);

            // Rak Tepi Kanan + Backstop Dorongan Kabel RJ45 (aman di kanan ESP32 X=105)
            translate([108.0, 95.0, wall - 0.2])
                cube([4.5, 28.0, 14.2]);
            translate([110.0, 95.0, wall + 13.8])
                cube([2.5, 28.0, 8.2]);

            // Saddle Penyangga W5500 Biru di Kiri-Atas
            translate([18.0, 96.0, wall - 0.2])
                cube([5.0, 26.0, 13.7]);
        }

        // Lubang pilot sekrup M3 pada keempat boss sudut
        for (p = boss_pts) {
            translate([p[0], p[1], body_h - 14.0])
                cylinder(d = 2.7, h = 15.5);
        }

        // Cutout Port RJ45 LAN W5500 Biru di Dinding KIRI (X=0)
        translate([-1.0, rj45_cy - rj45_w / 2, rj45_z0])
            cube([wall + 2.5, rj45_w, rj45_h]);

        // Cutout Port USB-C ESP32 di Dinding ATAS (Y=box_l)
        usbc_cutout();

        // 2x Lubang Keyhole Gantungan Dinding Baut M4 di Zona Tengah Terbuka (Z=0)
        for (kx = [24.0, 96.0]) {
            translate([kx, 62.0, -0.5]) cylinder(d = 8.6, h = wall + 1.5);
            translate([kx - 2.25, 62.0, -0.5]) cube([4.5, 9.0, wall + 1.5]);
            translate([kx, 62.0, wall - 1.0]) cylinder(d = 10.0, h = 1.5);
            translate([kx - 4.3, 62.0, wall - 1.0]) cube([8.6, 9.0, 1.5]);
        }

        // Ventilasi Samping Kiri & Kanan (zona tengah Y=56..74)
        for (vy = [56.0 : 6.0 : 74.0]) {
            translate([-1.0, vy, 22.0]) cube([wall + 2.0, 2.5, 12.0]);
            translate([box_w - wall - 1.0, vy, 22.0]) cube([wall + 2.0, 2.5, 12.0]);
        }
    }
}

// --- BAGIAN 2: TUTUP DEPAN / FRONT PANEL ---
// OLED tengah-atas, grill buzzer tengah, ceruk lengkung masuk jari AS608 di bawah-tengah.
module box_lid() {
    difference() {
        union() {
            // Pelat Muka Utama
            rounded_block(box_w, box_l, lid_h, corner_r);

            // Bibir Pengunci Keliling (dibebaskan di sudut boss M3 & di area ceruk AS608 bawah)
            difference() {
                translate([wall - 0.55, wall - 0.55, -2.2])
                    rounded_block(box_w - 2*(wall - 0.55), box_l - 2*(wall - 0.55), 2.4, corner_r - wall + 0.55);

                translate([wall + 1.05, wall + 1.05, -2.8])
                    rounded_block(box_w - 2*(wall + 1.05), box_l - 2*(wall + 1.05), 3.4, corner_r - wall - 1.05);

                for (p = boss_pts) {
                    translate([p[0], p[1], -3.0])
                        cylinder(d = 9.2, h = 3.5);
                }

                // Bebaskan bibir tutup agar tidak menabrak dinding terowongan AS608
                translate([as608_cx - 17.0, -1.0, -3.0])
                    cube([34.0, 51.0, 3.5]);
            }

            // Cradle Internal OLED 0.91" di TENGAH-ATAS (X=60.0, Y=96.0, PCB 38.6 x 12.6 mm)
            for (dy_wall = [-12.6 / 2 - 2.0, 12.6 / 2]) {
                translate([oled_cx - 38.6 / 2 - 2.0, oled_cy + dy_wall, -5.5])
                    cube([42.6, 2.0, 5.7]);
            }
            for (sx_end = [oled_cx - 38.6 / 2 - 2.0, oled_cx + 38.6 / 2]) {
                translate([sx_end, oled_cy - 12.6 / 2 - 2.0, -5.5])
                    cube([2.0, 3.8, 5.7]);
                translate([sx_end, oled_cy + 12.6 / 2 - 1.8, -5.5])
                    cube([2.0, 3.8, 5.7]);
            }
        }

        // 4x Lubang Sekrup Countersunk M3 Sudut
        for (p = boss_pts) {
            translate([p[0], p[1], -1.0])
                cylinder(d = 3.4, h = lid_h + 2.0);
            translate([p[0], p[1], lid_h - 2.2])
                cylinder(d1 = 3.4, d2 = 6.4, h = 2.5);
        }

        // A. Jendela Layar OLED 0.91" + Chamfer Bezel 45 deg
        translate([oled_cx - 26.5 / 2, oled_cy - 9.5 / 2, -6.0])
            cube([26.5, 9.5, lid_h + 7.0]);
        hull() {
            translate([oled_cx - 26.5 / 2, oled_cy - 9.5 / 2, lid_h - 2.0])
                cube([26.5, 9.5, 0.1]);
            translate([oled_cx - 30.5 / 2, oled_cy - 13.5 / 2, lid_h + 0.15])
                cube([30.5, 13.5, 0.1]);
        }

        // B. Pintu Ceruk Jari AS608 di Bawah-Tengah (X=46..74, Y=4.5..47.0; sisakan bibir penahan 2mm di Y=2.5..4.5)
        translate([as608_cx - 14.0, 4.5, -3.0])
            cube([28.0, 42.5, lid_h + 4.0]);
        // Chamfer luar 45 deg di kiri, kanan, dan atas ceruk masuk jari (bibir bawah tetap rata)
        hull() {
            translate([as608_cx - 14.0, 4.5, lid_h - 3.0])
                cube([28.0, 42.5, 0.1]);
            translate([as608_cx - 17.0, 4.5, lid_h + 0.15])
                cube([34.0, 45.5, 0.1]);
        }

        // C. Grill Akustik Buzzer 12mm (4 slot horizontal di tengah)
        for (i = [-1.5, -0.5, 0.5, 1.5]) {
            translate([buzz_cx - 6.0, buzz_cy + i * 3.2 - 0.8, -1.0])
                cube([12.0, 1.6, lid_h + 2.0]);
        }

        // D. Frame Dekoratif Terukir di Muka Depan (terputus rapi di sudut & di ceruk jari bawah)
        difference() {
            difference() {
                translate([7.5, 7.5, lid_h - 0.5])
                    rounded_block(box_w - 15.0, box_l - 15.0, 1.0, 3.0);
                translate([8.7, 8.7, lid_h - 0.7])
                    rounded_block(box_w - 17.4, box_l - 17.4, 1.5, 2.0);
            }
            for (p = boss_pts) {
                translate([p[0], p[1], lid_h - 1.0])
                    cylinder(d = 12.0, h = 2.5);
            }
            translate([as608_cx - 20.0, -1.0, lid_h - 1.0])
                cube([40.0, 54.0, 2.5]);
        }
    }
}

// --- PEMILIHAN OUTPUT PART ---
if (PART == "body") {
    box_body();
} else if (PART == "lid") {
    // Dibalik 180 deg menghadap bawah pada Z=0 -> siap cetak TANPA SUPPORT
    translate([0, box_l, lid_h])
        rotate([180, 0, 0])
        box_lid();
} else if (PART == "assembled") {
    box_body();
    translate([0, 0, body_h + ASSEMBLE_GAP])
        box_lid();
} else {
    // "preview": Bodi & Tutup berdampingan menghadap atas
    box_body();
    translate([box_w + 15.0, 0, 6.0])
        box_lid();
}
