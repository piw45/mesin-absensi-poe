// ============================================================================
// 3D PRINT ENCLOSURE - VARIAN 2: W5500 MINI HIJAU (HR961160C) KOMPAK
// MODEL PORTRAIT ALA ZKTECO (SESUAI SKETSA IMG_7761.HEIC):
//   - OLED 0.91" di TENGAH-ATAS panel depan
//   - AS608 posisi REBAHAN HORIZONTAL (memanjang 56mm ke dalam arah Z!) di ceruk
//     bawah-tengah (kantong jari 28 x 44.5 mm, kedalaman jari 26 mm, baffle rapi)
//   - Mainboard 50x70 (hingga 60x70 mm) di separuh ATAS bodi belakang OLED,
//     diputar 90 deg CCW agar USB-C & RJ45 keluar langsung dari dinding KANAN (+X)
// Mesin Absensi SIAKAD Denanyar (Unit C dari IMG_7760.MOV & IMG_7758.HEIC)
// ----------------------------------------------------------------------------
// Dimensi Luar Box  : 88.0 (X) x 140.0 (Y) x 68.0 (Z) mm (Body 63.5 + Lid 4.5)
// Export STL        : Set PART="body" atau PART="lid" (lid otomatis terbalik Z=0)
// ============================================================================

// Pilih mode render/export: "preview" | "assembled" | "body" | "lid"
PART = "preview";

// Lebar perfboard arah X saat diputar 90 deg CCW (50.0 untuk 5x7 cm ketat, 60.0 hingga 6x7 cm)
perf_l = 60.0;

// Jarak renggang Z untuk visualisasi PART="assembled" (0 = tertutup rapat)
ASSEMBLE_GAP = 0.0;

$fn = 64;

// --- PARAMETER DIMENSI UTAMA (mm) ---
box_w    = 88.0;   // Lebar luar X (proporsi portrait ramping sesuai sketsa IMG_7761)
box_l    = 140.0;  // Tinggi luar Y (ceruk AS608 bawah 49mm + bay perfboard 70mm di atas)
body_h   = 63.5;   // Kedalaman bodi Z (muat AS608 rebahan 56mm + ruang JST belakang)
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

// Parameter port dinding KANAN (X=box_w) -> sejajar berdampingan pitch 28.5 mm
usbc_cy  = 70.0;   // Pusat port USB-C ESP32
usbc_w   = 14.0;
usbc_h   = 11.0;
usbc_z0  = wall + 7.0;

rj45_cy  = 98.5;   // Pusat port RJ45 W5500 Mini
rj45_w   = 20.0;
rj45_h   = 18.0;
rj45_z0  = wall + 8.0;

// Parameter panel depan simetris (X = 44.0 mm)
as608_cx = 44.0;   // Pusat terowongan/ceruk jari AS608 di bawah tengah
oled_cx  = 44.0;   // Pusat OLED 0.91" di tengah atas
oled_cy  = 98.0;
buzz_cx  = 44.0;   // Pusat grill akustik buzzer
buzz_cy  = 68.0;

// --- MODUL BANTU GEOMETRI ---
module rounded_block(w, l, h, r) {
    hull() {
        for (x = [r, w - r])
            for (y = [r, l - r])
                translate([x, y, 0]) cylinder(r = r, h = h);
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

            // Rel pemandu tengah AS608 (lebar saluran dalam 24.4mm untuk bodi 23.5mm)
            translate([cx - 14.2, wall - 0.2, 12.0])
                cube([2.0, 18.5 - wall + 0.2, body_h - 12.0]);
            translate([cx + 12.2, wall - 0.2, 12.0])
                cube([2.0, 18.5 - wall + 0.2, body_h - 12.0]);

            // Plafon penjepit bodi kamera belakang AS608 (Y=25..27)
            translate([cx - 14.2, 25.0, wall - 0.2])
                cube([28.4, 2.0, 42.0 - wall + 0.2]);

            // Baffle penutup depan atas (Z=40..42, Y=26.5..47.5) dengan lereng 45 deg bebas support
            hull() {
                translate([cx - 14.2, 26.5, 40.0])
                    cube([28.4, 21.0, 2.0]);
                translate([cx - 14.2, 46.5, 19.5])
                    cube([28.4, 1.0, 2.0]);
            }

            // 2x Tiang stopper belakang AS608 pada Z=12.0 (tengah terbuka 18mm untuk JST 6-pin)
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
    backstop_x = 76.0 - perf_l;

    difference() {
        union() {
            // Dinding luar + lantai belakang berongga
            difference() {
                rounded_block(box_w, box_l, body_h, corner_r);

                // Rongga dalam (X=2.5..85.5, Y=2.5..137.5, Z=2.5..63.5)
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

            // Ceruk Terowongan AS608 Rebahan di Bawah-Tengah (X=44.0)
            as608_tunnel(as608_cx);

            // Bay Mainboard Atas (perfboard 70mm pada Y=53..123, duduk di Z_in=6.5mm -> ruang bawah 6.5mm)
            // Ledge Bawah + Rel Penahan Bawah (berjarak 2mm di atas atap terowongan Y=49)
            translate([18.0, 52.5, wall - 0.2])
                cube([55.0, 4.0, 6.7]);
            translate([18.0, 51.0, wall - 0.2])
                cube([55.0, 2.0, 8.7]);

            // Ledge Atas + Rel Penahan Atas (aman di bawah boss sudut atas Y=128)
            translate([18.0, 119.5, wall - 0.2])
                cube([55.0, 4.0, 6.7]);
            translate([18.0, 123.0, wall - 0.2])
                cube([55.0, 2.0, 8.7]);

            // Penyangga Sudut Kanan Perfboard (tumpang tindih volume di X=72.5..73.0, bebas dari port!)
            translate([72.5, 52.5, wall - 0.2])
                cube([3.5, 9.0, 6.7]);
            translate([72.5, 110.5, wall - 0.2])
                cube([3.5, 13.0, 6.7]);

            // Rak Kiri + Dinding Backstop Penahan Dorongan Kabel USB-C & RJ45
            translate([backstop_x - 3.0, 60.0, wall - 0.2])
                cube([6.0, 56.0, 6.7]);
            translate([backstop_x - 3.0, 60.0, wall + 6.3])
                cube([3.0, 56.0, 8.7]);
        }

        // Lubang pilot sekrup M3 pada keempat boss sudut
        for (p = boss_pts) {
            translate([p[0], p[1], body_h - 14.0])
                cylinder(d = 2.7, h = 15.5);
        }

        // Cutout Dinding KANAN (X=box_w): USB-C & RJ45 W5500 Mini Sejajar
        translate([box_w - wall - 1.0, usbc_cy - usbc_w / 2, usbc_z0])
            cube([wall + 2.5, usbc_w, usbc_h]);
        translate([box_w - wall - 1.0, rj45_cy - rj45_w / 2, rj45_z0])
            cube([wall + 2.5, rj45_w, rj45_h]);

        // 2x Lubang Keyhole Gantungan Dinding Baut M4 di Zona Atas (Z=0)
        for (kx = [24.0, 64.0]) {
            translate([kx, 125.5, -0.5]) cylinder(d = 8.6, h = wall + 1.5);
            translate([kx - 2.25, 125.5, -0.5]) cube([4.5, 8.5, wall + 1.5]);
            translate([kx, 125.5, wall - 1.0]) cylinder(d = 10.0, h = 1.2);
            translate([kx - 4.3, 125.5, wall - 1.0]) cube([8.6, 8.5, 1.2]);
        }

        // Ventilasi Samping (Kiri di zona tengah-atas, Kanan di zona bawah bebas port)
        for (vy = [62.0 : 6.0 : 80.0]) {
            translate([-1.0, vy, 24.0]) cube([wall + 2.0, 2.5, 12.0]);
        }
        for (vy = [20.0 : 6.0 : 38.0]) {
            translate([box_w - wall - 1.0, vy, 24.0]) cube([wall + 2.0, 2.5, 12.0]);
        }
    }
}

// --- BAGIAN 2: TUTUP DEPAN / FRONT PANEL ---
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

            // Cradle Internal OLED 0.91" di TENGAH-ATAS (X=44.0, Y=98.0, PCB 38.6 x 12.6 mm)
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

        // B. Pintu Ceruk Jari AS608 di Bawah-Tengah (X=30..58, Y=4.5..47.0; sisakan bibir penahan 2mm)
        translate([as608_cx - 14.0, 4.5, -3.0])
            cube([28.0, 42.5, lid_h + 4.0]);
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
