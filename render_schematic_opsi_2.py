import os
import math
import struct
from PIL import Image, ImageDraw, ImageFont

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
# PART 1: GENERATE DETAILED SCHEMATIC DIAGRAM FOR OPTION 2 (LAN8720A + PoE + XL7015)
# ==============================================================================
def render_schematic():
    SW, SH = 1600, 960
    img = Image.new("RGB", (SW, SH), (246, 249, 247))
    d = ImageDraw.Draw(img)

    f_title = get_font(28, bold=True)
    f_sub   = get_font(18, bold=False)
    f_box_h = get_font(18, bold=True)
    f_txt_b = get_font(15, bold=True)
    f_txt   = get_font(14, bold=False)
    f_pin   = get_font(13, bold=False, mono=True)

    # Header Bar
    d.rectangle([0, 0, SW, 82], fill=(16, 85, 48))
    d.text((40, 16), "SKEMATIK DETAIL OPSI 2: SOKET RJ45 SOLDER PCB + CHIP ETHERNET LAN8720A + STEPDOWN XL7015", fill=(255, 255, 255), font=f_title)
    d.text((40, 48), "Native RMII Hardware MAC (ESP32) | Soket MagJack PoE di PCB | Bridge Dioda + Signature 25kOhm | Input 48V-80V ke 5.0V", fill=(195, 245, 215), font=f_sub)

    # 1. RJ45 Socket J1 (Far Left: X=40..280, Y=115..580)
    d.rounded_rectangle([40, 115, 280, 580], radius=12, fill=(255, 255, 255), outline=(16, 85, 48), width=3)
    d.rectangle([40, 115, 280, 155], fill=(215, 240, 225))
    d.text((58, 126), "J1: SOKET RJ45 MAGJACK", fill=(16, 85, 48), font=f_box_h)

    rj_pins = [
        ("Pin 1 (TX+)", "Putih-Oranye [DATA]", (25, 95, 195)),
        ("Pin 2 (TX-)", "Oranye       [DATA]", (25, 95, 195)),
        ("Pin 3 (RX+)", "Putih-Hijau  [DATA]", (25, 95, 195)),
        ("Pin 6 (RX-)", "Hijau        [DATA]", (25, 95, 195)),
        ("Pin 4 (PoE+)", "Biru         [48V+]", (205, 40, 35)),
        ("Pin 5 (PoE+)", "Putih-Biru   [48V+]", (205, 40, 35)),
        ("Pin 7 (PoE-)", "Putih-Coklat [48V-]", (120, 65, 30)),
        ("Pin 8 (PoE-)", "Coklat       [48V-]", (120, 65, 30)),
    ]
    for i, (p, desc, col) in enumerate(rj_pins):
        py = 175 + i * 46
        d.text((52, py), p, fill=col, font=f_txt_b)
        d.text((145, py), desc, fill=(75, 90, 85), font=f_pin)

    # 2. LAN8720A Ethernet PHY Module (Upper-Center: X=360..740, Y=115..450)
    d.rounded_rectangle([360, 115, 740, 450], radius=12, fill=(255, 255, 255), outline=(35, 95, 220), width=3)
    d.rectangle([360, 115, 740, 155], fill=(215, 230, 255))
    d.text((385, 126), "U2: CHIP ETHERNET LAN8720A (RMII)", fill=(25, 75, 185), font=f_box_h)

    d.text((380, 172), "Koneksi Differential Pair dari J1:", fill=(25, 75, 185), font=f_txt_b)
    d.text((395, 196), "- TX+, TX-, RX+, RX- masuk ke LAN8720A", fill=(55, 70, 65), font=f_txt)
    d.text((380, 230), "Antarmuka RMII Hardware ESP32:", fill=(25, 75, 185), font=f_txt_b)
    rmii_pins = [
        "TX_EN -> IO21  |  TX0 -> IO19  |  TX1 -> IO22",
        "RX0   -> IO25  |  RX1 -> IO26  |  CRS -> IO27",
        "MDC   -> IO23  |  MDIO -> IO18 |  CLK -> IO0/IO17",
        "Supply VCC: +3.3V (LDO ESP32) | GND: Ground Bersama"
    ]
    for ri, rline in enumerate(rmii_pins):
        d.text((395, 258 + ri * 26), rline, fill=(45, 60, 55), font=f_pin)

    d.rounded_rectangle([380, 375, 720, 435], radius=6, fill=(240, 248, 255), outline=(130, 180, 240))
    d.text((392, 385), "KEUNGGULAN VS W5500: Native EMAC ESP32,", fill=(15, 80, 165), font=f_txt_b)
    d.text((392, 407), "Zero Overhead SPI, Kecepatan 10/100 Mbps Nyata!", fill=(15, 80, 165), font=f_txt)

    # 4 Data differential lines J1 -> LAN8720A
    for i in range(4):
        py = 182 + i * 46
        d.line([(280, py), (360, py)], fill=(35, 95, 220), width=3)
        d.ellipse([357, py - 3, 363, py + 3], fill=(35, 95, 220))

    # 3. PoE Bridge Rectifier D1..D4 + 25k Signature (Lower-Center: X=360..740, Y=490..850)
    d.rounded_rectangle([360, 490, 740, 850], radius=12, fill=(255, 255, 255), outline=(215, 115, 5), width=3)
    d.rectangle([360, 490, 740, 530], fill=(255, 242, 205))
    d.text((385, 501), "SISTEM PoE AKTIF IEEE 802.3af (48V)", fill=(175, 80, 5), font=f_box_h)

    poe_notes = [
        ("1. Jembatan Dioda D1-D4 (1N4007 / MB10S):", True),
        ("   - Pasangan Pin 4 & 5 digabung -> Input AC1", False),
        ("   - Pasangan Pin 7 & 8 digabung -> Input AC2", False),
        ("   - Mencegah polaritas terbalik (T568A / T568B)", False),
        ("2. Signature Resistor 25.0 kOhm Presisi:", True),
        ("   - Susunan: R1(10K) + R2(10K) + (R3||R4 10K)", False),
        ("   - Menghasilkan tepat 25.0 kOhm paralel V_POE", False),
        ("   - Switch PoE aktif (HSAirPo/Hikvision) otomatis", False),
        ("     mendeteksi beban & mengalirkan listrik 48V!", False),
    ]
    py_c = 545
    for ptext, is_bold in poe_notes:
        d.text((380, py_c), ptext, fill=(175, 80, 5) if is_bold else (55, 65, 60), font=f_txt_b if is_bold else f_txt)
        py_c += 28

    # Power lines from J1 (pins 4,5 and 7,8) to Bridge
    d.line([(280, 366), (320, 366), (320, 600), (360, 600)], fill=(205, 40, 35), width=3)
    d.line([(280, 412), (320, 412)], fill=(205, 40, 35), width=3)
    d.line([(280, 458), (305, 458), (305, 630), (360, 630)], fill=(120, 65, 30), width=3)
    d.line([(280, 504), (305, 504)], fill=(120, 65, 30), width=3)

    # 4. XL7015 Step-Down Module (Mid-Right: X=810..1120, Y=490..850)
    d.rounded_rectangle([810, 490, 1120, 850], radius=12, fill=(255, 255, 255), outline=(215, 35, 35), width=3)
    d.rectangle([810, 490, 1120, 530], fill=(255, 225, 225))
    d.text((830, 501), "U3: STEP-DOWN XL7015 (80V->5V)", fill=(175, 25, 25), font=f_box_h)

    xl_notes = [
        "- Tegangan Input: 48V - 57V DC dari Bridge",
        "- Rating Tegangan IC: s/d 80V DC (Super Aman)",
        "- Kapasitor Filter Input: 47uF - 100uF / 63V",
        "- Output Tegangan: 5.00V DC Stabil (max 1.2A)",
        "- Proteksi Dioda D5 (1N4007) pada VOUT+",
        "",
        "PETUNJUK KALIBRASI WAJIB:",
        "Putar trimpot biru XL7015 sambil ukur",
        "multimeter sampai tepat 5.00V DC sebelum",
        "menyolder/memasang ESP32 & LAN8720A!"
    ]
    py_c = 548
    for xln in xl_notes:
        is_warn = "WAJIB" in xln or "Putar" in xln or "5.00V" in xln
        d.text((828, py_c), xln, fill=(185, 25, 25) if is_warn else (50, 65, 60), font=f_txt_b if is_warn else f_txt)
        py_c += 27

    # 48V lines from Bridge to XL7015
    d.line([(740, 610), (810, 610)], fill=(205, 40, 35), width=4)
    d.text((752, 588), "+48V DC", fill=(205, 40, 35), font=f_txt_b)
    d.line([(740, 660), (810, 660)], fill=(30, 40, 50), width=4)
    d.text((758, 666), "GND", fill=(30, 40, 50), font=f_txt_b)

    # 5. ESP32 DevKit V1 + Peripherals Box (Far Right: X=1180..1560, Y=115..880)
    d.rounded_rectangle([1180, 115, 1560, 880], radius=12, fill=(255, 255, 255), outline=(16, 85, 48), width=3)
    d.rectangle([1180, 115, 1560, 155], fill=(215, 240, 225))
    d.text((1205, 126), "U1: ESP32 + OLED + AS608 + BUZZER", fill=(16, 85, 48), font=f_box_h)

    esp_sections = [
        ("1. Catu Daya Masuk:", [
            "+5.0V (dari XL7015) -> Pin VIN (5V) ESP32",
            "+3.3V (LDO ESP32)   -> Suplai LAN8720, OLED, AS608",
            "GND Bersama Seluruh Modul"
        ]),
        ("2. Bus RMII ke LAN8720A:", [
            "TX_EN:IO21 | TX0:IO19 | TX1:IO22",
            "RX0:IO25   | RX1:IO26 | CRS_DV:IO27",
            "MDC:IO23   | MDIO:IO18| REFCLK:IO0"
        ]),
        ("3. Layar OLED 0.91\" I2C (128x32):", [
            "SDA -> IO4  |  SCL -> IO2",
            "VCC -> 3.3V |  GND -> GND"
        ]),
        ("4. Sensor Sidik Jari AS608 (UART2):", [
            "TX -> IO16(RX2) | RX -> IO17(TX2)",
            "TCH -> IO35 (Touch Wakeup) + R5 10K ke GND",
            "V+ & VA -> 3.3V | GND -> GND"
        ]),
        ("5. Buzzer Aktif 3.3V (BZ1):", [
            "IO14 -> R6 1K -> BZ1(+) (+ Dioda D6 1N4007)",
            "BZ1(-) -> GND"
        ])
    ]
    py_c = 170
    for stitle, slines in esp_sections:
        d.text((1200, py_c), stitle, fill=(16, 85, 48), font=f_txt_b)
        py_c += 24
        for sline in slines:
            d.text((1215, py_c), sline, fill=(45, 60, 55), font=f_pin)
            py_c += 22
        py_c += 10

    # 5V line from XL7015 to ESP32 VIN
    d.line([(1120, 610), (1150, 610), (1150, 205), (1180, 205)], fill=(225, 40, 40), width=4)
    d.text((1125, 586), "+5.0V", fill=(225, 40, 40), font=f_txt_b)
    # GND line from XL7015 to ESP32 GND
    d.line([(1120, 660), (1140, 660), (1140, 240), (1180, 240)], fill=(30, 40, 50), width=4)
    d.text((1130, 666), "GND", fill=(30, 40, 50), font=f_txt_b)
    # RMII bus bundle indicator between LAN8720A and ESP32
    d.line([(740, 270), (1180, 270)], fill=(35, 95, 220), width=4)
    d.text((910, 245), "<=== BUS RMII ETHERNET (9 Jalur) ===>", fill=(35, 95, 220), font=f_txt_b)

    # Footer banner
    d.rectangle([0, SH - 45, SW, SH], fill=(22, 34, 46))
    d.text((40, SH - 32), "SIAKAD MA'HAD ALY DENANYAR - RANCANGAN HARDWARE MESIN ABSENSI PoE OPSI 2 (LAN8720A NATIVE RMII)", fill=(200, 225, 245), font=f_sub)

    out_img = img.resize((1280, 768), Image.Resampling.LANCZOS)
    p1 = os.path.join(OUT_DIR, "skematik_poe_opsi_b.png")
    p2 = os.path.join(ART_DIR, "skematik_poe_opsi_b.png")
    out_img.save(p1)
    out_img.save(p2)
    return p1

if __name__ == "__main__":
    p = render_schematic()
    print("SCHEMATIC OK:", p)
