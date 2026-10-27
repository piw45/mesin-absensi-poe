import os
import math
import struct
from PIL import Image, ImageDraw, ImageFont
from generate_clean_pcb import write_kicad_pcb, render_pcb_preview

OUT_DIR = r"C:\Users\arifi\denanyar.ac.id\hardware\mesin-absensi-poe"
ART_DIR = r"C:\Users\arifi\.gemini\antigravity-cli\brain\6f53efb7-e9da-4589-8dc3-60bfeda03a17"
os.makedirs(OUT_DIR, exist_ok=True)
os.makedirs(ART_DIR, exist_ok=True)

def get_font(size, bold=False, mono=False):
    candidates = []
    if mono:
        candidates = ["consolab.ttf" if bold else "consola.ttf"]
    else:
        candidates = ["segoeuib.ttf" if bold else "segoeui.ttf", "arialbd.ttf" if bold else "arial.ttf"]
    for c in candidates:
        p = os.path.join(r"C:\Windows\Fonts", c)
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()

# ==============================================================================
# OPTION 2: RJ45 SOLDER PCB + CHIP ETHERNET LAN8720A (RMII) + STEPDOWN XL7015
# DIMENSI: PORTRAIT 80.0 x 100.0 mm (SINKRON 1:1 DENGAN CASING 88 x 108 x 38 mm)
# ==============================================================================
PCB_W = 80.0
PCB_H = 100.0
BOX_W = 88.0
BOX_L = 108.0
BOX_BODY_H = 33.0
BOX_LID_H = 5.0
WALL = 2.5

MOUNT_HOLES = [
    ("H1", 4.0, 4.0),
    ("H2", 76.0, 4.0),
    ("H3", 4.0, 96.0),
    ("H4", 76.0, 96.0),
]

NETS = {
    0: "",
    1: "GND",
    2: "+5V",
    3: "+3V3",
    4: "V_POE_48V",
    5: "POE_PAIR_45",
    6: "POE_PAIR_78",
    7: "ETH_TXP",
    8: "ETH_TXN",
    9: "ETH_RXP",
    10: "ETH_RXN",
    11: "RMII_TX_EN",
    12: "RMII_TX0",
    13: "RMII_TX1",
    14: "RMII_RX0",
    15: "RMII_RX1",
    16: "RMII_CRS_DV",
    17: "RMII_MDC",
    18: "RMII_MDIO",
    19: "RMII_REFCLK",
    20: "I2C_SDA",
    21: "I2C_SCL",
    22: "FP_TX",
    23: "FP_RX",
    24: "FP_TCH",
    25: "BUZ_IO14",
    26: "BUZ_POS",
    27: "POE_SIG_A",
    28: "POE_SIG_B",
    29: "XL_VOUT_RAW",
}

PADS = []
def add_pad(comp, pad_num, x, y, net_name, label="", shape="circle", size=1.75, drill=1.0):
    PADS.append({
        "comp": comp,
        "pad": str(pad_num),
        "x": round(x, 2),
        "y": round(y, 2),
        "net": net_name,
        "label": label or str(pad_num),
        "shape": shape,
        "size": size,
        "drill": drill,
    })

# 1. J1: OLED 0.91" 128x32 I2C (Top-Center: X=40.0, Y=10.5; Courtyard X=21..59, Y=4.5..16.8)
oled_pins = [
    ("1", "GND", "GND"),
    ("2", "VCC", "+3V3"),
    ("3", "SCL", "I2C_SCL"),
    ("4", "SDA", "I2C_SDA"),
]
for i, (pnum, lbl, net) in enumerate(oled_pins):
    add_pad("J1_OLED", pnum, 36.19 + i * 2.54, 15.2, net, lbl, shape="rect" if i == 0 else "circle")

# 2. U1: ESP32 DevKit V1 (30-Pin) - Upper/Mid Left: X_left=5.2, X_right=30.6 (25.4mm), Y=20.0..55.56
# Pinout configured for Hardware RMII EMAC + OLED + AS608 + Buzzer
esp_left = [
    ("1", "3V3", "+3V3"),
    ("2", "EN", ""),
    ("3", "VP", ""),
    ("4", "VN", ""),
    ("5", "IO34", ""),
    ("6", "IO35", "FP_TCH"),
    ("7", "IO32", ""),
    ("8", "IO33", ""),
    ("9", "IO25", "RMII_RX0"),
    ("10", "IO26", "RMII_RX1"),
    ("11", "IO27", "RMII_CRS_DV"),
    ("12", "IO14", "BUZ_IO14"),
    ("13", "IO12", ""),
    ("14", "GND", "GND"),
    ("15", "VIN", "+5V"),
]
esp_right = [
    ("30", "GND", "GND"),
    ("29", "IO23", "RMII_MDC"),
    ("28", "IO22", "RMII_TX1"),
    ("27", "TX0", ""),
    ("26", "RX0", ""),
    ("25", "IO21", "RMII_TX_EN"),
    ("24", "GND", "GND"),
    ("23", "IO19", "RMII_TX0"),
    ("22", "IO18", "RMII_MDIO"),
    ("21", "IO5", ""),
    ("20", "IO17", "FP_RX"),
    ("19", "IO16", "FP_TX"),
    ("18", "IO4", "I2C_SDA"),
    ("17", "IO2", "I2C_SCL"),
    ("16", "IO0", "RMII_REFCLK"),
]
for i, (pnum, lbl, net) in enumerate(esp_left):
    add_pad("U1_ESP32", pnum, 5.2, 20.0 + i * 2.54, net, lbl, shape="rect" if i == 0 else "circle")
for i, (pnum, lbl, net) in enumerate(esp_right):
    add_pad("U1_ESP32", pnum, 30.6, 20.0 + i * 2.54, net, lbl)

# 3. U2: Modul Ethernet LAN8720A (RMII) (Upper-Right: X=48.5..75.5, Y=19.0..45.0)
# 2x6 Header untuk LAN8720A Breakout Board
lan8720_header = [
    ("1",  50.0, 22.00, "+3V3",        "VCC"),    ("2",  52.54, 22.00, "GND",         "GND"),
    ("3",  50.0, 24.54, "RMII_TX_EN",  "TX_EN"),  ("4",  52.54, 24.54, "RMII_TX0",    "TX0"),
    ("5",  50.0, 27.08, "RMII_TX1",    "TX1"),    ("6",  52.54, 27.08, "RMII_RX0",    "RX0"),
    ("7",  50.0, 29.62, "RMII_RX1",    "RX1"),    ("8",  52.54, 29.62, "RMII_CRS_DV", "CRS"),
    ("9",  50.0, 32.16, "RMII_MDC",    "MDC"),    ("10", 52.54, 32.16, "RMII_MDIO",   "MDIO"),
    ("11", 50.0, 34.70, "RMII_REFCLK", "CLK"),    ("12", 52.54, 34.70, "+3V3",        "RST"),
]
for pnum, px, py, net, lbl in lan8720_header:
    add_pad("U2_LAN8720A", pnum, px, py, net, lbl, shape="rect" if pnum == "1" else "circle")

# Input differential pair pads pada modul LAN8720A
lan8720_diff = [
    ("E1", 57.00, 42.5, "ETH_TXP", "TX+"),
    ("E2", 59.54, 42.5, "ETH_TXN", "TX-"),
    ("E3", 62.08, 42.5, "ETH_RXP", "RX+"),
    ("E6", 64.62, 42.5, "ETH_RXN", "RX-"),
]
for pnum, px, py, net, lbl in lan8720_diff:
    add_pad("U2_LAN8720A", pnum, px, py, net, lbl)

# 4. J2: AS608 Sensor 6-Pin Header (X=33.65..46.35, Y=59.5, right above AS608 Bay)
as608_pins = [
    ("1", "V+", "+3V3"),
    ("2", "TX", "FP_TX"),
    ("3", "RX", "FP_RX"),
    ("4", "GND", "GND"),
    ("5", "TCH", "FP_TCH"),
    ("6", "VA", "+3V3"),
]
for i, (pnum, lbl, net) in enumerate(as608_pins):
    add_pad("J2_AS608", pnum, 33.65 + i * 2.54, 59.5, net, lbl, shape="rect" if i == 0 else "circle")

# 5. J_RJ45_IN: 8-Pin On-PCB RJ45 MagJack at Bottom-Center (X_center = 40.0, Y = 89.5..92.04)
rj45_pins = [
    ("1", 35.56, 89.50, "ETH_TXP",     "1:TX+"),
    ("2", 36.83, 92.04, "ETH_TXN",     "2:TX-"),
    ("3", 38.10, 89.50, "ETH_RXP",     "3:RX+"),
    ("4", 39.37, 92.04, "POE_PAIR_45", "4:PoE1"),
    ("5", 40.64, 89.50, "POE_PAIR_45", "5:PoE1"),
    ("6", 41.91, 92.04, "ETH_RXN",     "6:RX-"),
    ("7", 43.18, 89.50, "POE_PAIR_78", "7:PoE2"),
    ("8", 44.45, 92.04, "POE_PAIR_78", "8:PoE2"),
]
for pnum, px, py, net, lbl in rj45_pins:
    add_pad("J_RJ45_IN", pnum, px, py, net, lbl, shape="rect" if pnum == "1" else "circle", size=1.5, drill=0.9)
add_pad("J_RJ45_IN", "M1", 32.38, 94.50, "", "MNT", size=3.6, drill=2.5)
add_pad("J_RJ45_IN", "M2", 47.62, 94.50, "", "MNT", size=3.6, drill=2.5)

# 6. PoE Bridge Rectifier D1..D4 (1N4007 / MB10S) + D5 + D6
diodes = [
    ("D1", 56.0, 74.0, "POE_PAIR_45", 63.62, 74.0, "V_POE_48V"),
    ("D2", 56.0, 78.0, "POE_PAIR_78", 63.62, 78.0, "V_POE_48V"),
    ("D3", 56.0, 82.0, "GND",         63.62, 82.0, "POE_PAIR_45"),
    ("D4", 56.0, 86.0, "GND",         63.62, 86.0, "POE_PAIR_78"),
    ("D5", 56.0, 48.5, "XL_VOUT_RAW", 63.62, 48.5, "+5V"),
    ("D6", 12.19, 76.0, "GND",        19.81, 76.0, "BUZ_POS"),
]
for dname, ax, ay, anet, kx, ky, knet in diodes:
    add_pad(dname, "A", ax, ay, anet, "A", shape="circle", size=1.8, drill=1.0)
    add_pad(dname, "K", kx, ky, knet, "K", shape="rect",   size=1.8, drill=1.0)

# 7. IEEE 802.3af 25.0kOhm Signature Resistors (R1..R4 = 10K) + R5 (10K TCH) + R6 (1K Buzzer)
resistors = [
    ("R1_10K", 67.5, 74.0, "V_POE_48V", 75.12, 74.0, "POE_SIG_A"),
    ("R2_10K", 67.5, 78.0, "POE_SIG_A", 75.12, 78.0, "POE_SIG_B"),
    ("R3_10K", 67.5, 82.0, "POE_SIG_B", 75.12, 82.0, "GND"),
    ("R4_10K", 67.5, 86.0, "POE_SIG_B", 75.12, 86.0, "GND"),
    ("R5_10K", 36.19, 54.5, "FP_TCH",   43.81, 54.5, "GND"),
    ("R6_1K",  12.19, 70.0, "BUZ_IO14", 19.81, 70.0, "BUZ_POS"),
]
for rname, x1, y1, n1, x2, y2, n2 in resistors:
    add_pad(rname, "1", x1, y1, n1, "1", size=1.6, drill=0.8)
    add_pad(rname, "2", x2, y2, n2, "2", size=1.6, drill=0.8)

# 8. U3: XL7015 Step-Down Module (48V -> 5.0V DC) at X=55.5..73.5, Y=52.0..68.0
add_pad("U3_XL7015", "VIN+",  69.5, 65.5, "V_POE_48V",   "VIN+",  shape="rect", size=2.2, drill=1.2)
add_pad("U3_XL7015", "VIN-",  58.5, 65.5, "GND",         "VIN-",  shape="circle", size=2.2, drill=1.2)
add_pad("U3_XL7015", "VOUT+", 58.5, 53.5, "XL_VOUT_RAW", "VOUT+", shape="rect", size=2.2, drill=1.2)
add_pad("U3_XL7015", "VOUT-", 69.5, 53.5, "GND",         "VOUT-", shape="circle", size=2.2, drill=1.2)

# 9. BZ1: Active Buzzer 3.3V at Bottom-Left X=16.0, Y=84.0
add_pad("BZ1", "+", 19.81, 84.0, "BUZ_POS", "+", shape="rect", size=1.8, drill=1.0)
add_pad("BZ1", "-", 12.19, 84.0, "GND",     "-", shape="circle", size=1.8, drill=1.0)

# PCB routing and preview are imported from generate_clean_pcb.py (single source of truth)





# ==============================================================================
# 3. WRITE PARAMETRIC OPENSCAD FILE (box_absensi.scad)
# ==============================================================================
def write_openscad():
    scad = """// ============================================================================
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
"""
    scad_path = os.path.join(OUT_DIR, "box_absensi.scad")
    with open(scad_path, "w", encoding="utf-8") as f:
        f.write(scad)
    return scad_path

# ==============================================================================
# 4. GENERATE HIGH-RESOLUTION 3D STL MESHES (box_absensi_body.stl & lid.stl)
# ==============================================================================
class MeshBuilder:
    def __init__(self):
        self.tris = []

    def add_tri(self, p1, p2, p3):
        ux, uy, uz = p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]
        vx, vy, vz = p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2]
        nx = uy*vz - uz*vy
        ny = uz*vx - ux*vz
        nz = ux*vy - uy*vx
        l = math.sqrt(nx*nx + ny*ny + nz*nz)
        if l > 1e-9:
            nx, ny, nz = nx/l, ny/l, nz/l
        self.tris.append(((nx, ny, nz), p1, p2, p3))

    def add_quad(self, p1, p2, p3, p4):
        self.add_tri(p1, p2, p3)
        self.add_tri(p1, p3, p4)

    def add_box(self, x0, y0, z0, x1, y1, z1):
        p000, p100, p110, p010 = (x0,y0,z0), (x1,y0,z0), (x1,y1,z0), (x0,y1,z0)
        p001, p101, p111, p011 = (x0,y0,z1), (x1,y0,z1), (x1,y1,z1), (x0,y1,z1)
        self.add_quad(p000, p010, p110, p100)
        self.add_quad(p001, p101, p111, p011)
        self.add_quad(p000, p100, p101, p001)
        self.add_quad(p100, p110, p111, p101)
        self.add_quad(p110, p010, p011, p111)
        self.add_quad(p010, p000, p001, p011)

    def add_hollow_cylinder(self, cx, cy, z0, z1, r_out, r_in, segs=32):
        for i in range(segs):
            a0 = 2 * math.pi * i / segs
            a1 = 2 * math.pi * (i + 1) / segs
            c0, s0 = math.cos(a0), math.sin(a0)
            c1, s1 = math.cos(a1), math.sin(a1)
            ox0, oy0 = cx + r_out*c0, cy + r_out*s0
            ox1, oy1 = cx + r_out*c1, cy + r_out*s1
            ix0, iy0 = cx + r_in*c0,  cy + r_in*s0
            ix1, iy1 = cx + r_in*c1,  cy + r_in*s1
            self.add_quad((ox0,oy0,z0), (ox1,oy1,z0), (ox1,oy1,z1), (ox0,oy0,z1))
            self.add_quad((ix1,iy1,z0), (ix0,iy0,z0), (ix0,iy0,z1), (ix1,iy1,z1))
            self.add_quad((ix0,iy0,z1), (ox0,oy0,z1), (ox1,oy1,z1), (ix1,iy1,z1))
            self.add_quad((ox0,oy0,z0), (ix0,iy0,z0), (ix1,iy1,z0), (ox1,oy1,z0))

    def add_rounded_rect_ring(self, w, l, z0, z1, r_out, thick, corner_segs=8):
        centers = [
            (r_out, r_out, math.pi, 1.5*math.pi),
            (w - r_out, r_out, 1.5*math.pi, 2.0*math.pi),
            (w - r_out, l - r_out, 0.0, 0.5*math.pi),
            (r_out, l - r_out, 0.5*math.pi, math.pi),
        ]
        r_in = max(0.5, r_out - thick)
        pts_out, pts_in = [], []
        for cx, cy, a_start, a_end in centers:
            for s in range(corner_segs + 1):
                a = a_start + (a_end - a_start) * s / corner_segs
                pts_out.append((cx + r_out * math.cos(a), cy + r_out * math.sin(a)))
                pts_in.append((cx + r_in * math.cos(a), cy + r_in * math.sin(a)))
        n = len(pts_out)
        for i in range(n):
            j = (i + 1) % n
            ox0, oy0 = pts_out[i]
            ox1, oy1 = pts_out[j]
            ix0, iy0 = pts_in[i]
            ix1, iy1 = pts_in[j]
            self.add_quad((ox0,oy0,z0), (ox1,oy1,z0), (ox1,oy1,z1), (ox0,oy0,z1))
            self.add_quad((ix1,iy1,z0), (ix0,iy0,z0), (ix0,iy0,z1), (ix1,iy1,z1))
            self.add_quad((ix0,iy0,z1), (ox0,oy0,z1), (ox1,oy1,z1), (ix1,iy1,z1))
            self.add_quad((ox0,oy0,z0), (ix0,iy0,z0), (ix1,iy1,z0), (ox1,oy1,z0))

    def save_binary_stl(self, path, header_str="MESIN_ABSENSI_POE_CAD"):
        with open(path, "wb") as f:
            hdr = header_str.encode("ascii")[:80].ljust(80, b" ")
            f.write(hdr)
            f.write(struct.pack("<I", len(self.tris)))
            for (nx, ny, nz), p1, p2, p3 in self.tris:
                f.write(struct.pack("<12fH",
                    nx, ny, nz,
                    p1[0], p1[1], p1[2],
                    p2[0], p2[1], p2[2],
                    p3[0], p3[1], p3[2], 0))

def build_stl_files():
    body = MeshBuilder()
    body.add_box(2.5, 2.5, 0.0, BOX_W - 2.5, BOX_L - 2.5, WALL)
    body.add_rounded_rect_ring(BOX_W, BOX_L, 0.0, 8.5, 5.0, WALL, corner_segs=10)
    body.add_box(0.0, 0.0, 8.5, 35.6, WALL, 23.0)
    body.add_box(52.4, 0.0, 8.5, BOX_W, WALL, 23.0)
    body.add_box(0.0, BOX_L - WALL, 8.5, BOX_W, BOX_L, 23.0)
    body.add_box(0.0, WALL, 8.5, WALL, BOX_L - WALL, 23.0)
    body.add_box(BOX_W - WALL, WALL, 8.5, BOX_W, BOX_L - WALL, 23.0)
    body.add_rounded_rect_ring(BOX_W, BOX_L, 23.0, BOX_BODY_H, 5.0, WALL, corner_segs=10)

    for sx in (8.0, 80.0):
        for sy in (8.0, 100.0):
            body.add_hollow_cylinder(sx, sy, WALL, WALL + 6.0, 4.0, 1.4, segs=32)
            body.add_box(sx - 1.0, sy - 4.2, WALL, sx + 1.0, sy + 4.2, WALL + 3.5)
            body.add_box(sx - 4.2, sy - 1.0, WALL, sx + 4.2, sy + 1.0, WALL + 3.5)

    body.add_box(32.0, 20.0, WALL, 56.0, 22.0, WALL + 8.5)
    body.add_box(32.0, 40.0, WALL, 56.0, 42.0, WALL + 8.5)
    body.add_box(32.0, 22.0, WALL, 34.0, 40.0, WALL + 8.5)
    body.add_box(54.0, 22.0, WALL, 56.0, 40.0, WALL + 8.5)

    body_path = os.path.join(OUT_DIR, "box_absensi_body.stl")
    body.save_binary_stl(body_path, "DENANYAR_ABSENSI_POE_BODY_88x108x33")

    lid = MeshBuilder()
    lid.add_rounded_rect_ring(BOX_W, BOX_L, 0.0, BOX_LID_H, 5.0, 4.0, corner_segs=10)
    lid.add_rounded_rect_ring(BOX_W - 4.0, BOX_L - 4.0, -2.5, 0.0, 3.0, 1.8, corner_segs=8)

    lid.add_box(3.5, 98.75, 0.0, BOX_W - 3.5, BOX_L - 3.5, BOX_LID_H)
    lid.add_box(3.5, 88.25, 0.0, 30.5, 98.75, BOX_LID_H)
    lid.add_box(57.5, 88.25, 0.0, BOX_W - 3.5, 98.75, BOX_LID_H)
    lid.add_box(3.5, 40.5,  0.0, BOX_W - 3.5, 88.25, BOX_LID_H)
    lid.add_box(3.5, 21.5,  0.0, 33.4, 40.5, BOX_LID_H)
    lid.add_box(54.6, 21.5, 0.0, BOX_W - 3.5, 40.5, BOX_LID_H)
    lid.add_box(3.5, 3.5,   0.0, BOX_W - 3.5, 21.5, BOX_LID_H)

    lid.add_box(31.5, 19.5, -13.5, 56.5, 21.5, 0.0)
    lid.add_box(31.5, 40.5, -13.5, 56.5, 42.5, 0.0)
    lid.add_box(31.5, 21.5, -13.5, 33.4, 40.5, 0.0)
    lid.add_box(54.6, 21.5, -13.5, 56.5, 40.5, 0.0)

    lid_path = os.path.join(OUT_DIR, "box_absensi_lid.stl")
    lid.save_binary_stl(lid_path, "DENANYAR_ABSENSI_POE_LID_88x108x5")
    return body_path, lid_path, len(body.tris), len(lid.tris)

# ==============================================================================
# 5. RENDER 3D ISOMETRIC EXPLODED ASSEMBLY + ORTHOGRAPHIC BLUEPRINT (box_3d_preview.png)
# ==============================================================================
def render_3d_box_preview():
    W, H = 2200, 1550
    img = Image.new("RGB", (W, H), (13, 18, 26))
    d = ImageDraw.Draw(img)

    f_title = get_font(32, bold=True)
    f_sub   = get_font(20, bold=False)
    f_sec   = get_font(21, bold=True)
    f_lbl   = get_font(15, bold=True, mono=True)
    f_small = get_font(14, bold=False, mono=True)

    d.text((50, 28), "DESAIN 3D ENCLOSURE OPSI 2 (LAN8720A RMII) & SINKRONISASI MEKANIS (1:1)", fill=(255, 255, 255), font=f_title)
    d.text((50, 70), "Dimensi Casing: 88.0 x 108.0 x 38.0 mm | Dimensi PCB: 80.0 x 100.0 mm | Standoff M3: 72.0 x 92.0 mm | Port RJ45 MagJack PoE", fill=(110, 225, 185), font=f_sub)

    # LEFT PANEL: 3D ISOMETRIC EXPLODED STACK
    d.rounded_rectangle([40, 115, 1120, H - 35], radius=16, fill=(19, 26, 37), outline=(60, 88, 125), width=2)
    d.text((68, 134), "1. 3D ISOMETRIC EXPLODED ASSEMBLY (URUTAN RAKITAN PRESISI 1:1)", fill=(255, 220, 100), font=f_sec)
    d.text((68, 164), "Garis putus-putus vertikal membuktikan lubang baut M3, OLED, AS608, Buzzer, & RJ45 sejajar 100%", fill=(175, 205, 235), font=f_small)

    def iso(x, y, z, cx=595, cy=1395, scale=4.15):
        ix = cx + (x - y) * math.cos(math.radians(28)) * scale
        iy = cy - (x + y) * math.sin(math.radians(28)) * scale - z * scale
        return (int(round(ix)), int(round(iy)))

    def draw_iso_prism(x0, y0, z0, x1, y1, z1, col_top, col_left, col_right, outline=(220, 235, 250)):
        p001 = iso(x0, y0, z1)
        p101 = iso(x1, y0, z1)
        p111 = iso(x1, y1, z1)
        p011 = iso(x0, y1, z1)
        p000 = iso(x0, y0, z0)
        p100 = iso(x1, y0, z0)
        p010 = iso(x0, y1, z0)
        d.polygon([p000, p100, p101, p001], fill=col_right, outline=outline)
        d.polygon([p010, p000, p001, p011], fill=col_left, outline=outline)
        d.polygon([p001, p101, p111, p011], fill=col_top, outline=outline)

    def draw_dashed_line(p1, p2, col, seg=10):
        dist = math.hypot(p2[0] - p1[0], p2[1] - p1[1])
        steps = max(2, int(dist // seg))
        for i in range(0, steps, 2):
            t0 = i / steps
            t1 = min(1.0, (i + 1) / steps)
            xa = int(p1[0] + (p2[0] - p1[0]) * t0)
            ya = int(p1[1] + (p2[1] - p1[1]) * t0)
            xb = int(p1[0] + (p2[0] - p1[0]) * t1)
            yb = int(p1[1] + (p2[1] - p1[1]) * t1)
            d.line([(xa, ya), (xb, yb)], fill=col, width=2)

    Z_BODY = 0.0
    Z_PCB  = 74.0
    Z_LID  = 148.0

    # LAYER A: 3D BOTTOM ENCLOSURE BODY
    draw_iso_prism(0, 0, Z_BODY, BOX_W, BOX_L, Z_BODY + 24, (42, 62, 86), (28, 44, 64), (35, 54, 76), outline=(100, 165, 230))
    c0, c1, c2, c3 = iso(3, 3, Z_BODY+24), iso(BOX_W-3, 3, Z_BODY+24), iso(BOX_W-3, BOX_L-3, Z_BODY+24), iso(3, BOX_L-3, Z_BODY+24)
    d.polygon([c0, c1, c2, c3], fill=(22, 34, 48), outline=(80, 135, 195))
    draw_iso_prism(35.6, 0, Z_BODY + 6, 52.4, 3.0, Z_BODY + 20, (240, 190, 50), (180, 130, 20), (14, 20, 28), outline=(255, 215, 80))
    rj_lbl_p = iso(44, 0, Z_BODY + 4)
    d.text((rj_lbl_p[0] + 24, rj_lbl_p[1] + 4), "<-- PORT RJ45 MagJack PoE (X=44mm)", fill=(255, 220, 90), font=f_lbl)

    for sx, sy in [(8, 8), (80, 8), (8, 100), (80, 100)]:
        draw_iso_prism(sx-2.5, sy-2.5, Z_BODY+20, sx+2.5, sy+2.5, Z_BODY+28, (235, 195, 75), (175, 140, 45), (205, 165, 55), outline=(255, 235, 130))

    d.text((64, 1280), "LAPIS 1 (BAWAH): BODI CASING 3D\n- 88.0 x 108.0 x 33.0 mm\n- 4x Tiang Standoff M3 (72x92mm)\n- 2x Keyhole Dinding M4", fill=(110, 195, 255), font=f_lbl)

    for sx, sy in [(8, 8), (80, 8), (8, 100), (80, 100)]:
        draw_dashed_line(iso(sx, sy, Z_BODY + 28), iso(sx, sy, Z_PCB), (255, 215, 80))

    # LAYER B: 3D ROUTED PCB (80 x 100 mm)
    draw_iso_prism(4.0, 4.0, Z_PCB, 84.0, 104.0, Z_PCB + 3.0, (20, 98, 56), (14, 68, 38), (18, 82, 46), outline=(120, 240, 165))
    # 1. ESP32 DevKit V1 (X_box=7.5..36.3, Y_box=46.5..85.8)
    draw_iso_prism(7.5, 46.5, Z_PCB + 3, 36.3, 85.8, Z_PCB + 11, (35, 42, 52), (25, 30, 38), (30, 36, 45), outline=(210, 220, 235))
    # 2. LAN8720A Ethernet Module (X_box=52.5..79.5, Y_box=59..85)
    draw_iso_prism(52.5, 59.0, Z_PCB + 3, 79.5, 85.0, Z_PCB + 12, (30, 95, 205), (20, 65, 150), (25, 80, 175), outline=(140, 205, 255))
    # 3. OLED 0.91" Module (X_box=25..63, Y_box=87.5..99.5)
    draw_iso_prism(25.0, 87.5, Z_PCB + 3, 63.0, 99.5, Z_PCB + 14, (15, 115, 165), (10, 80, 120), (12, 95, 140), outline=(0, 230, 255))
    # 4. AS608 Cutout Bay & Optical Sensor Prism (X_box=33..55, Y_box=21..41)
    draw_iso_prism(33.0, 21.0, Z_PCB - 6, 55.0, 41.0, Z_PCB + 18, (25, 165, 110), (18, 115, 75), (22, 140, 92), outline=(110, 255, 185))
    # 5. XL7015 Step-Down (X_box=59.5..77.5, Y_box=36..53)
    draw_iso_prism(59.5, 36.0, Z_PCB + 3, 77.5, 53.0, Z_PCB + 11, (185, 40, 40), (135, 25, 25), (160, 30, 30), outline=(255, 150, 150))
    # 6. J_RJ45_IN MagJack at Bottom Edge (X_box=36..52, Y_box=3.5..17.5)
    draw_iso_prism(36.0, 3.5, Z_PCB + 3, 52.0, 17.5, Z_PCB + 15, (215, 180, 55), (165, 135, 35), (190, 155, 45), outline=(255, 235, 120))
    # 7. Buzzer BZ1 (X_box=15..25, Y_box=15..25)
    draw_iso_prism(15.0, 15.0, Z_PCB + 3, 25.0, 25.0, Z_PCB + 11, (40, 40, 45), (25, 25, 30), (32, 32, 38), outline=(220, 220, 230))

    d.text((64, 860), "LAPIS 2 (TENGAH): PCB 80.0 x 100.0 mm\n- LAN8720A RMII (Native ESP32)\n- Duduk presisi di atas 4x Standoff M3\n- J_RJ45_IN tepat di mulut lubang bawah\n- Sensor AS608 masuk ke Bay tengah PCB", fill=(110, 245, 175), font=f_lbl)

    for sx, sy in [(8, 8), (80, 8), (8, 100), (80, 100)]:
        draw_dashed_line(iso(sx, sy, Z_PCB + 3), iso(sx, sy, Z_LID), (255, 215, 80))
    draw_dashed_line(iso(44, 93.5, Z_PCB + 14), iso(44, 93.5, Z_LID), (0, 235, 255))
    draw_dashed_line(iso(44, 31.0, Z_PCB + 18), iso(44, 31.0, Z_LID), (80, 255, 160))
    draw_dashed_line(iso(20, 20.0, Z_PCB + 11), iso(20, 20.0, Z_LID), (255, 150, 190))

    # LAYER C: 3D TOP LID / FRONT PANEL
    draw_iso_prism(0, 0, Z_LID, BOX_W, BOX_L, Z_LID + 6.0, (52, 74, 102), (34, 50, 72), (42, 62, 88), outline=(140, 205, 255))
    o0, o1, o2, o3 = iso(30.5, 88.25, Z_LID+6), iso(57.5, 88.25, Z_LID+6), iso(57.5, 98.75, Z_LID+6), iso(30.5, 98.75, Z_LID+6)
    d.polygon([o0, o1, o2, o3], fill=(8, 28, 42), outline=(0, 235, 255))
    f0, f1, f2, f3 = iso(30.0, 18.0, Z_LID+6), iso(58.0, 18.0, Z_LID+6), iso(58.0, 44.0, Z_LID+6), iso(30.0, 44.0, Z_LID+6)
    d.polygon([f0, f1, f2, f3], fill=(32, 48, 66), outline=(110, 245, 180))
    fa0, fa1, fa2, fa3 = iso(33.4, 21.5, Z_LID+6), iso(54.6, 21.5, Z_LID+6), iso(54.6, 40.5, Z_LID+6), iso(33.4, 40.5, Z_LID+6)
    d.polygon([fa0, fa1, fa2, fa3], fill=(12, 68, 44), outline=(80, 255, 160))
    for gi in (-3, 0, 3):
        g0, g1 = iso(14.5, 20.0 + gi, Z_LID + 6), iso(25.5, 20.0 + gi, Z_LID + 6)
        d.line([g0, g1], fill=(255, 175, 200), width=4)

    d.text((64, 440), "LAPIS 3 (ATAS): TUTUP CASING (LID)\n- Jendela Bezel OLED (X=44, Y=93.5mm)\n- Cekungan Jari AS608 (X=44, Y=31.0mm)\n- Grill Suara Buzzer (X=20, Y=20.0mm)", fill=(0, 230, 255), font=f_lbl)

    # RIGHT PANEL: 2D ORTHOGRAPHIC ENGINEERING DRAWINGS
    d.rounded_rectangle([1150, 115, W - 40, H - 35], radius=16, fill=(19, 26, 37), outline=(60, 88, 125), width=2)
    d.text((1180, 134), "2. BLUEPRINT TEKNIK ORTOGONAL & OVERLAY PCB DI DALAM BOX", fill=(255, 220, 100), font=f_sec)
    d.text((1180, 164), "Skala milimeter presisi antara PCB (Hijau) dan Box (Biru) - Opsi 2 (LAN8720A RMII)", fill=(175, 205, 235), font=f_small)

    SO = 6.2
    OX_TOP = 1235
    OY_TOP = 260
    def p_top(x_box, y_box):
        return (int(round(OX_TOP + x_box * SO)), int(round(OY_TOP + (BOX_L - y_box) * SO)))

    tb0, tb1 = p_top(0, BOX_L), p_top(BOX_W, 0)
    d.rounded_rectangle([tb0[0], tb0[1], tb1[0], tb1[1]], radius=28, fill=(28, 40, 56), outline=(90, 175, 255), width=3)
    ti0, ti1 = p_top(WALL, BOX_L - WALL), p_top(BOX_W - WALL, WALL)
    d.rounded_rectangle([ti0[0], ti0[1], ti1[0], ti1[1]], radius=16, fill=(16, 24, 34), outline=(65, 120, 180), width=2)
    tp0, tp1 = p_top(4.0, 104.0), p_top(84.0, 4.0)
    d.rectangle([tp0[0], tp0[1], tp1[0], tp1[1]], fill=(16, 68, 40), outline=(80, 245, 150), width=3)

    for sx, sy in [(8, 8), (80, 8), (8, 100), (80, 100)]:
        scx, scy = p_top(sx, sy)
        d.ellipse([scx - 18, scy - 18, scx + 18, scy + 18], fill=(215, 175, 60), outline=(255, 235, 130), width=2)
        d.ellipse([scx - 8, scy - 8, scx + 8, scy + 8], fill=(16, 24, 34), outline=(255, 255, 255), width=1)

    # 1. OLED (X_box=44, Y_box=93.5, 27x10.5mm)
    ol0, ol1 = p_top(44 - 13.5, 93.5 + 5.25), p_top(44 + 13.5, 93.5 - 5.25)
    d.rectangle([ol0[0]-18, ol0[1]-5, ol1[0]+18, ol1[1]+5], outline=(100, 200, 230), width=1)
    d.rectangle([ol0[0], ol0[1], ol1[0], ol1[1]], fill=(8, 35, 52), outline=(0, 235, 255), width=3)
    d.text((ol0[0] + 6, ol0[1] + 22), "OLED 0.91\" (X=44, Y=93.5)", fill=(0, 235, 255), font=f_small)

    # 2. ESP32 (Left: X_box=7.5..36.3, Y_box=46.5..85.8) & LAN8720A (Right: X_box=52.5..79.5, Y_box=59..85)
    e0, e1 = p_top(7.5, 85.8), p_top(36.3, 46.5)
    d.rectangle([e0[0], e0[1], e1[0], e1[1]], fill=(28, 38, 48), outline=(200, 215, 230), width=2)
    d.text((e0[0] + 14, e0[1] + 95), "ESP32 30P\n(Kiri-Atas)", fill=(220, 235, 250), font=f_small)

    w0, w1 = p_top(52.5, 85.0), p_top(79.5, 59.0)
    d.rectangle([w0[0], w0[1], w1[0], w1[1]], fill=(20, 55, 115), outline=(130, 195, 255), width=2)
    d.text((w0[0] + 14, w0[1] + 55), "LAN8720A\n(RMII MAC)", fill=(190, 225, 255), font=f_small)

    # 3. AS608 Sensor & Cutout Bay (Center-Bottom: X_box=33..55, Y_box=21..41)
    ab0, ab1 = p_top(33.0, 41.0), p_top(55.0, 21.0)
    d.rectangle([ab0[0], ab0[1], ab1[0], ab1[1]], fill=(14, 42, 30), outline=(80, 255, 160), width=3)
    d.text((ab0[0] + 8, ab0[1] + 40), "AS608 BAY\n(X=44, Y=31)", fill=(110, 255, 185), font=f_small)

    # 4. XL7015 & PoE Bridge (Lower-Right: X_box=58.5..80.5, Y_box=16..53)
    p0, p1 = p_top(58.5, 53.0), p_top(80.5, 16.0)
    d.rectangle([p0[0], p0[1], p1[0], p1[1]], fill=(45, 25, 30), outline=(255, 140, 150), width=2)
    d.text((p0[0] + 8, p0[1] + 75), "XL7015 +\nBridge PoE", fill=(255, 200, 210), font=f_small)

    # 5. Buzzer + Grill (Lower-Left: X_box=20, Y_box=20)
    bzc = p_top(20.0, 20.0)
    bzr = int(round(6.0 * SO))
    d.ellipse([bzc[0]-bzr, bzc[1]-bzr, bzc[0]+bzr, bzc[1]+bzr], fill=(42, 35, 48), outline=(255, 175, 205), width=2)
    d.text((bzc[0]-34, bzc[1]+bzr+6), "BZ1+Grill", fill=(255, 175, 205), font=f_small)

    # 6. RJ45 MagJack Port (Bottom-Center: X_box=35.6..52.4, Y_box=-1..17.5)
    rj0, rj1 = p_top(35.6, 17.5), p_top(52.4, -1.0)
    d.rectangle([rj0[0], rj0[1], rj1[0], rj1[1]], fill=(195, 155, 40), outline=(255, 230, 100), width=3)
    d.text((rj0[0] - 25, rj1[1] + 10), "PORT RJ45 PoE (X=44.0 mm)", fill=(255, 230, 100), font=f_lbl)

    d.text(((tb0[0]+tb1[0])//2 - 110, tb0[1] - 32), "<--- LEBAR BOX: 88.0 mm (PCB: 80.0 mm) --->", fill=(90, 195, 255), font=f_lbl)
    d.text((tb1[0] + 16, (tb0[1]+tb1[1])//2 - 40), "TINGGI BOX:\n108.0 mm\n\nTINGGI PCB:\n100.0 mm\n\nJARAK BAUT:\n72 x 92 mm", fill=(90, 195, 255), font=f_lbl)

    # BOTTOM-RIGHT: SIDE CROSS-SECTION VIEW
    cs_x0, cs_y0 = 1185, 1015
    cs_x1, cs_y1 = W - 65, H - 60
    d.rounded_rectangle([cs_x0, cs_y0, cs_x1, cs_y1], radius=12, fill=(24, 34, 48), outline=(80, 120, 170), width=2)
    d.text((cs_x0 + 22, cs_y0 + 16), "3. POTONGAN SAMPING (CROSS-SECTION Z-STACK - TINGGI TOTAL 38.0 mm)", fill=(255, 220, 100), font=f_sec)

    zx0 = cs_x0 + 60
    zy_bot = cs_y1 - 65
    d.rectangle([zx0, zy_bot - 18, zx0 + 560, zy_bot], fill=(55, 85, 120), outline=(130, 195, 255), width=2)
    d.rectangle([zx0, zy_bot - 220, zx0 + 22, zy_bot], fill=(55, 85, 120), outline=(130, 195, 255), width=2)
    d.rectangle([zx0 + 538, zy_bot - 220, zx0 + 560, zy_bot], fill=(55, 85, 120), outline=(130, 195, 255), width=2)
    d.rectangle([zx0, zy_bot - 255, zx0 + 560, zy_bot - 220], fill=(70, 105, 145), outline=(160, 220, 255), width=2)
    d.rectangle([zx0 + 38, zy_bot - 65, zx0 + 70, zy_bot - 18], fill=(215, 175, 60), outline=(255, 230, 120), width=2)
    d.rectangle([zx0 + 490, zy_bot - 65, zx0 + 522, zy_bot - 18], fill=(215, 175, 60), outline=(255, 230, 120), width=2)
    d.rectangle([zx0 + 30, zy_bot - 78, zx0 + 220, zy_bot - 65], fill=(22, 115, 62), outline=(100, 255, 165), width=2)
    d.rectangle([zx0 + 340, zy_bot - 78, zx0 + 530, zy_bot - 65], fill=(22, 115, 62), outline=(100, 255, 165), width=2)
    d.rectangle([zx0 + 226, zy_bot - 255, zx0 + 334, zy_bot - 45], fill=(24, 150, 98), outline=(110, 255, 185), width=3)
    d.text((zx0 + 235, zy_bot - 165), "SENSOR\nAS608\n(Tebal 21.5mm\nmasuk ke lubang\nbay tengah PCB)", fill=(235, 255, 245), font=f_small)
    d.rectangle([zx0 + 95, zy_bot - 220, zx0 + 185, zy_bot - 195], fill=(12, 125, 180), outline=(0, 235, 255), width=2)
    d.rectangle([zx0 + 130, zy_bot - 195, zx0 + 150, zy_bot - 78], fill=(190, 190, 200))
    d.text((zx0 + 88, zy_bot - 188), "OLED (Pin Header)", fill=(0, 235, 255), font=f_small)
    d.rectangle([zx0 + 370, zy_bot - 180, zx0 + 485, zy_bot - 78], fill=(195, 155, 45), outline=(255, 230, 110), width=2)
    d.text((zx0 + 378, zy_bot - 145), "RJ45 + LAN8720\n+ XL7015", fill=(255, 245, 200), font=f_small)

    tx = zx0 + 585
    ty = cs_y0 + 58
    z_notes = [
        "RINCIAN TINGGI Z (38.0 mm):",
        "- Lantai Bawah Box : 2.5 mm",
        "- Tiang Standoff M3: 6.0 mm",
        "- Tebal Papan PCB  : 1.6 mm",
        "- Ruang Modul Atas : 22.9 mm",
        "- Tebal Tutup (Lid): 5.0 mm",
        "-----------------------------",
        "STL High-Poly Mesh:",
        "- box_absensi_body.stl",
        "- box_absensi_lid.stl",
        "Siap cetak di Cura / Bambu!"
    ]
    for zn in z_notes:
        d.text((tx, ty), zn, fill=(215, 235, 255), font=f_small)
        ty += 25

    out_img = img.resize((1375, 968), Image.Resampling.LANCZOS)
    p1 = os.path.join(OUT_DIR, "box_3d_preview.png")
    p2 = os.path.join(ART_DIR, "box_3d_preview.png")
    out_img.save(p1)
    out_img.save(p2)
    return p1

if __name__ == "__main__":
    pcb_file = write_kicad_pcb()
    pcb_img  = render_pcb_preview()
    scad_file = write_openscad()
    stl_body, stl_lid, n_body, n_lid = build_stl_files()
    box_img  = render_3d_box_preview()
    assert os.path.exists(pcb_file) and os.path.getsize(pcb_file) > 10000
    assert n_body > 400 and n_lid > 200
    print(f"SUCCESS OPSI 2: PCB={pcb_file} ({os.path.getsize(pcb_file)} B), BodySTL={n_body} tris, LidSTL={n_lid} tris")
