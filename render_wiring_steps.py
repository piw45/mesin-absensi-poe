import os
from PIL import Image, ImageDraw, ImageFont

PROJ_DIR = r"C:\Users\Laptop\mesin-absensi-poe"
ART_DIR = r"C:\Users\Laptop\.gemini\antigravity\brain\ce03850e-f42c-4152-bcd2-05e2c82a49f5"
os.makedirs(PROJ_DIR, exist_ok=True)
os.makedirs(ART_DIR, exist_ok=True)

def get_font(size, bold=False, mono=False):
    font_paths = [
        "C:/Windows/Fonts/arialbd.ttf" if bold else "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeuib.ttf" if bold else "C:/Windows/Fonts/segoeui.ttf",
    ]
    if mono:
        font_paths.insert(0, "C:/Windows/Fonts/consola.ttf" if bold else "C:/Windows/Fonts/consolab.ttf")
    for p in font_paths:
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()

def save_both(img, filename):
    p1 = os.path.join(PROJ_DIR, filename)
    p2 = os.path.join(ART_DIR, filename)
    img.save(p1, quality=95)
    img.save(p2, quality=95)
    print("Saved:", p1)

def draw_wire(d, points, color, width=6, label=None, label_pos=None, labels_queue=None):
    for i in range(len(points) - 1):
        d.line([points[i], points[i+1]], fill=(15, 23, 42), width=width+4)
    for i in range(len(points) - 1):
        d.line([points[i], points[i+1]], fill=color, width=width)
    for pt in [points[0], points[-1]]:
        r = width + 2
        d.ellipse([pt[0]-r, pt[1]-r, pt[0]+r, pt[1]+r], fill=color, outline=(15, 23, 42), width=2)
    if label and label_pos:
        if labels_queue is not None:
            labels_queue.append((label, label_pos, color))
        else:
            draw_badge(d, label, label_pos, color)

def draw_badge(d, label, label_pos, color):
    f = get_font(15, bold=True)
    bbox = d.textbbox(label_pos, label, font=f)
    pad = 7
    d.rounded_rectangle([bbox[0]-pad, bbox[1]-pad, bbox[2]+pad, bbox[3]+pad], radius=6, fill=(255, 255, 255), outline=color, width=3)
    d.text(label_pos, label, fill=(15, 23, 42), font=f)

def flush_labels(d, labels_queue):
    for label, label_pos, color in labels_queue:
        draw_badge(d, label, label_pos, color)

def draw_esp32(d, x, y, active_left=None, active_right=None):
    """
    Menggambar PERSIS ESP32 30-Pin USB Type-C milik Arif (Sesuai Foto IMG_7735):
    - Sisi Kiri (Atas ke Bawah): EN, VP, VN, D34, D35, D32, D33, D25, D26, D27, D14, D12, D13, GND, VIN
    - Sisi Kanan (Atas ke Bawah): D23, D22, TX0, RX0, D21, D19, D18, D5, D17, D16, D4, D2, D15, GND, 3V3
    """
    active_left = active_left or []
    active_right = active_right or []
    w, h = 390, 735
    d.rounded_rectangle([x, y, x+w, y+h], radius=18, fill=(20, 30, 48), outline=(71, 85, 105), width=4)
    # Antena PCB atas
    d.rounded_rectangle([x+75, y+8, x+w-75, y+50], radius=6, fill=(30, 41, 59), outline=(100, 116, 139), width=2)
    # Kaleng metal ESP-WROOM-32
    d.rounded_rectangle([x+75, y+52, x+w-75, y+175], radius=10, fill=(180, 190, 205), outline=(226, 232, 240), width=3)
    d.text((x+115, y+82), "ESP-32 (MILIKMU)", fill=(15, 23, 42), font=get_font(18, bold=True))
    d.text((x+100, y+112), "Sesuai Foto IMG_7735", fill=(185, 28, 28), font=get_font(15, bold=True))
    d.text((x+108, y+138), "(30-Pin USB Type-C)", fill=(51, 65, 85), font=get_font(14, bold=True))

    # USB Type-C bawah + tombol EN & BOOT
    d.rounded_rectangle([x+80, y+h-45, x+115, y+h-15], radius=4, fill=(71, 85, 105), outline=(203, 213, 225), width=2)
    d.text((x+86, y+h-37), "EN", fill=(255,255,255), font=get_font(12, bold=True))
    d.rounded_rectangle([x+w-115, y+h-45, x+w-80, y+h-15], radius=4, fill=(71, 85, 105), outline=(203, 213, 225), width=2)
    d.text((x+w-113, y+h-37), "BOOT", fill=(255,255,255), font=get_font(10, bold=True))

    d.rounded_rectangle([x+130, y+h-48, x+w-130, y+h+10], radius=10, fill=(203, 213, 225), outline=(100, 116, 139), width=3)
    d.text((x+142, y+h-32), "USB TYPE-C", fill=(15, 23, 42), font=get_font(14, bold=True))
    d.text((x+95, y+h-72), "PATOKAN BAWAH (USB-C)", fill=(250, 204, 21), font=get_font(15, bold=True))

    # PERSIS SESUAI TULISAN PUTIH DI FOTO IMG_7735 MILIK ARIF!
    left_pins  = ["EN", "VP", "VN", "D34", "D35", "D32", "D33", "D25", "D26", "D27", "D14", "D12", "D13", "GND_L", "VIN"]
    right_pins = ["D23", "D22", "TX0", "RX0", "D21", "D19", "D18", "D5", "D17", "D16", "D4", "D2", "D15", "GND_R", "3V3"]

    coords = {}
    py_start = y + 200
    pitch = 31
    f_pin = get_font(16, bold=True, mono=True)

    for i, pname in enumerate(left_pins):
        py = py_start + i * pitch
        is_act = pname in active_left
        pad_col = (250, 204, 21) if is_act else (71, 85, 105)
        txt_col = (255, 255, 255) if is_act else (148, 163, 184)
        if is_act:
            d.rounded_rectangle([x-8, py-13, x+132, py+13], radius=6, fill=(185, 28, 28), outline=(250, 204, 21), width=2)
        d.rectangle([x+12, py-8, x+32, py+8], fill=pad_col, outline=(255,255,255) if is_act else None)
        disp_name = "GND" if pname.startswith("GND") else pname
        d.text((x+40, py-10), f"{i+1:02d}. {disp_name}", fill=txt_col, font=f_pin)
        coords[pname] = (x+12, py)

    for i, pname in enumerate(right_pins):
        py = py_start + i * pitch
        is_act = pname in active_right
        pad_col = (250, 204, 21) if is_act else (71, 85, 105)
        txt_col = (255, 255, 255) if is_act else (148, 163, 184)
        if is_act:
            d.rounded_rectangle([x+w-132, py-13, x+w+8, py+13], radius=6, fill=(185, 28, 28), outline=(250, 204, 21), width=2)
        d.rectangle([x+w-32, py-8, x+w-12, py+8], fill=pad_col, outline=(255,255,255) if is_act else None)
        disp_name = "GND" if pname.startswith("GND") else pname
        d.text((x+w-122, py-10), f"{disp_name:>3} .{i+1:02d}", fill=txt_col, font=f_pin)
        coords[pname] = (x+w-12, py)

    return coords


# ==============================================================================
# GAMBAR 1: STEP 1 - RANGKAIAN DIODA BRIDGE, RESISTOR 25K & KALIBRASI XL7015
# ==============================================================================
def make_step1():
    W, H = 1600, 1050
    img = Image.new("RGB", (W, H), (248, 250, 252))
    d = ImageDraw.Draw(img)
    lbls = []

    d.rectangle([0, 0, W, 90], fill=(185, 28, 28))
    d.text((35, 15), "STEP 1: CARA MERAKIT DIODA, RESISTOR 25K & PUTAR XL7015 KE 5.00V", fill=(255, 255, 255), font=get_font(28, bold=True))
    d.text((35, 53), "PERHATIAN: Pada tahap ini, JANGAN pasang ESP32 dulu! Kita setel voltase 48V jadi 5.00V terlebih dahulu.", fill=(254, 226, 226), font=get_font(18, bold=True))

    # 1. Kabel LAN PoE Box
    d.rounded_rectangle([35, 130, 315, 480], radius=14, fill=(255, 255, 255), outline=(30, 41, 59), width=3)
    d.rectangle([35, 130, 315, 180], fill=(226, 232, 240))
    d.text((55, 145), "1. UJUNG KABEL LAN", fill=(15, 23, 42), font=get_font(18, bold=True))
    d.text((50, 200), "Kupas sedikit kabel LAN atau", fill=(51, 65, 85), font=get_font(15))
    d.text((50, 222), "ambil dari kaki bawah RJ45:", fill=(51, 65, 85), font=get_font(15))

    d.rounded_rectangle([50, 265, 300, 340], radius=8, fill=(254, 226, 226), outline=(220, 38, 38), width=2)
    d.text((62, 275), "Kabel Biru & Putih-Biru", fill=(185, 28, 28), font=get_font(15, bold=True))
    d.text((62, 300), "(Pin 4 & Pin 5 digabung)", fill=(153, 27, 27), font=get_font(14, bold=True))
    pt_lan_45 = (300, 302)

    d.rounded_rectangle([50, 370, 300, 445], radius=8, fill=(241, 245, 249), outline=(71, 85, 105), width=2)
    d.text((62, 380), "Kabel Coklat & Pth-Coklat", fill=(15, 23, 42), font=get_font(15, bold=True))
    d.text((62, 405), "(Pin 7 & Pin 8 digabung)", fill=(51, 65, 85), font=get_font(14, bold=True))
    pt_lan_78 = (300, 407)

    # 2. PCB Bolong 4x6cm
    d.rounded_rectangle([365, 130, 885, 695], radius=14, fill=(254, 249, 195), outline=(202, 138, 4), width=3)
    d.rectangle([365, 130, 885, 180], fill=(253, 224, 71))
    d.text((385, 145), "2. RAKIT DI PCB BOLONG 4x6 CM", fill=(113, 63, 18), font=get_font(18, bold=True))

    def draw_diode_horiz(dx, dy, name, desc):
        d.line([(dx-45, dy), (dx+125, dy)], fill=(100, 116, 139), width=5)
        d.rectangle([dx, dy-16, dx+80, dy+16], fill=(15, 23, 42), outline=(0,0,0), width=2)
        d.rectangle([dx+60, dy-16, dx+76, dy+16], fill=(226, 232, 240))
        d.text((dx+8, dy-9), name, fill=(255, 255, 255), font=get_font(14, bold=True))
        d.text((dx-35, dy-35), desc, fill=(15, 23, 42), font=get_font(12, bold=True))
        return (dx-45, dy), (dx+125, dy)

    d1_a, d1_k = draw_diode_horiz(480, 260, "D1", "D1 (Gelang Perak di Kanan ->)")
    d2_a, d2_k = draw_diode_horiz(480, 340, "D2", "D2 (Gelang Perak di Kanan ->)")
    d3_a, d3_k = draw_diode_horiz(480, 430, "D3", "D3 (Gelang Perak di Kanan ->)")
    d4_a, d4_k = draw_diode_horiz(480, 510, "D4", "D4 (Gelang Perak di Kanan ->)")

    draw_wire(d, [d1_k, (650, 260), (650, 340), d2_k], (220, 38, 38), width=5)
    draw_wire(d, [d3_a, (405, 430), (405, 510), d4_a], (15, 23, 42), width=5)

    draw_wire(d, [pt_lan_45, (425, 302), (425, 260), d1_a], (234, 88, 12), width=5)
    draw_wire(d, [(425, 302), (425, 380), (620, 380), (620, 430), d3_k], (234, 88, 12), width=5)

    draw_wire(d, [pt_lan_78, (385, 407), (385, 340), d2_a], (120, 53, 15), width=5)
    draw_wire(d, [(385, 407), (385, 550), (620, 550), (620, 510), d4_k], (120, 53, 15), width=5)

    d.rounded_rectangle([695, 240, 865, 530], radius=10, fill=(255, 255, 255), outline=(16, 185, 129), width=3)
    d.text((708, 250), "RESISTOR 25K", fill=(6, 95, 70), font=get_font(15, bold=True))
    d.text((708, 272), "(4x Resistor 10k):", fill=(6, 95, 70), font=get_font(13, bold=True))
    d.text((708, 300), "Ujung Atas (+48V)", fill=(220, 38, 38), font=get_font(12, bold=True))
    d.text((708, 320), "   |", fill=(15, 23, 42), font=get_font(13, bold=True))
    d.text((708, 340), "[R1: 10k Ohm]", fill=(15, 23, 42), font=get_font(13, bold=True))
    d.text((708, 360), "   |", fill=(15, 23, 42), font=get_font(13, bold=True))
    d.text((708, 380), "[R2: 10k Ohm]", fill=(15, 23, 42), font=get_font(13, bold=True))
    d.text((708, 400), "   +-----+", fill=(15, 23, 42), font=get_font(13, bold=True))
    d.text((708, 420), "[R3:10k] [R4:10k]", fill=(15, 23, 42), font=get_font(12, bold=True))
    d.text((708, 440), "(R3 & R4 didobel)", fill=(71, 85, 105), font=get_font(12))
    d.text((708, 460), "   +-----+", fill=(15, 23, 42), font=get_font(13, bold=True))
    d.text((708, 480), "Ujung Bawah (GND)", fill=(15, 23, 42), font=get_font(12, bold=True))

    draw_wire(d, [(650, 280), (695, 280)], (220, 38, 38), width=5)
    draw_wire(d, [(405, 510), (405, 585), (780, 585), (780, 530)], (15, 23, 42), width=5)

    d.text((385, 628), "CARA BACA DIODA 1N4007:", fill=(185, 28, 28), font=get_font(14, bold=True))
    d.text((385, 648), "Semua 4 dioda gelang warna PERAK-nya", fill=(15, 23, 42), font=get_font(13, bold=True))
    d.text((385, 668), "menghadap ke KANAN!", fill=(15, 23, 42), font=get_font(13, bold=True))

    # 3. Modul Step-Down XL7015
    xl_x, xl_y = 945, 195
    d.rounded_rectangle([xl_x, xl_y, xl_x+310, xl_y+380], radius=14, fill=(22, 101, 52), outline=(20, 83, 45), width=4)
    d.text((xl_x+35, xl_y+20), "3. MODUL XL7015", fill=(255, 255, 255), font=get_font(22, bold=True))
    d.text((xl_x+35, xl_y+50), "(Lihat tulisan di balik PCB)", fill=(187, 247, 208), font=get_font(14, bold=True))

    pt_vin_p  = (xl_x+25, xl_y+110)
    pt_vin_m  = (xl_x+25, xl_y+320)
    pt_vout_p = (xl_x+285, xl_y+110)
    pt_vout_m = (xl_x+285, xl_y+320)

    for pt, lbl, align_r in [
        (pt_vin_p, "IN+", False),
        (pt_vin_m, "IN-", False),
        (pt_vout_p, "OUT+", True),
        (pt_vout_m, "OUT-", True)
    ]:
        d.ellipse([pt[0]-16, pt[1]-16, pt[0]+16, pt[1]+16], fill=(250, 204, 21), outline=(255, 255, 255), width=3)
        tx = pt[0]-80 if align_r else pt[0]+25
        d.text((tx, pt[1]-12), lbl, fill=(255, 255, 255), font=get_font(20, bold=True))

    d.rounded_rectangle([xl_x+90, xl_y+150, xl_x+220, xl_y+265], radius=8, fill=(37, 99, 235), outline=(255, 255, 255), width=3)
    d.ellipse([xl_x+130, xl_y+172, xl_x+180, xl_y+222], fill=(234, 179, 8), outline=(255, 255, 255), width=2)
    d.line([xl_x+140, xl_y+197, xl_x+170, xl_y+197], fill=(15, 23, 42), width=4)
    d.text((xl_x+98, xl_y+234), "BAUT KUNINGAN", fill=(255, 255, 255), font=get_font(12, bold=True))

    draw_wire(d, [(650, 280), (650, 200), (910, 200), (910, pt_vin_p[1]), pt_vin_p], (220, 38, 38), width=6, label="Kabel Merah (+48V ke IN+)", label_pos=(665, 155), labels_queue=lbls)
    draw_wire(d, [(780, 585), (910, 585), (910, pt_vin_m[1]), pt_vin_m], (15, 23, 42), width=6, label="Kabel Hitam (GND ke IN-)", label_pos=(665, 605), labels_queue=lbls)

    # 4. Multimeter Digital
    mm_x, mm_y = 1320, 195
    d.rounded_rectangle([mm_x, mm_y, mm_x+245, mm_y+420], radius=18, fill=(245, 158, 11), outline=(180, 83, 9), width=4)
    d.text((mm_x+35, mm_y+15), "MULTIMETER", fill=(15, 23, 42), font=get_font(22, bold=True))
    d.rounded_rectangle([mm_x+20, mm_y+50, mm_x+225, mm_y+135], radius=8, fill=(187, 247, 208), outline=(22, 101, 52), width=3)
    d.text((mm_x+35, mm_y+72), "5.00 V", fill=(6, 78, 59), font=get_font(40, bold=True, mono=True))
    d.text((mm_x+25, mm_y+155), "Setel Putaran ke:", fill=(15, 23, 42), font=get_font(15, bold=True))
    d.text((mm_x+25, mm_y+180), "20V DC (V=)", fill=(185, 28, 28), font=get_font(20, bold=True))

    pt_mm_red = (mm_x+65, mm_y+380)
    pt_mm_blk = (mm_x+185, mm_y+380)
    d.ellipse([pt_mm_red[0]-15, pt_mm_red[1]-15, pt_mm_red[0]+15, pt_mm_red[1]+15], fill=(220, 38, 38), outline=(255,255,255), width=2)
    d.ellipse([pt_mm_blk[0]-15, pt_mm_blk[1]-15, pt_mm_blk[0]+15, pt_mm_blk[1]+15], fill=(15, 23, 42), outline=(255,255,255), width=2)

    draw_wire(d, [pt_mm_red, (mm_x+65, mm_y+460), (1290, mm_y+460), (1290, pt_vout_p[1]), pt_vout_p], (220, 38, 38), width=6, label="Jarum Merah (+) ke OUT+", label_pos=(1150, 140), labels_queue=lbls)
    draw_wire(d, [pt_mm_blk, (mm_x+185, mm_y+495), (xl_x+230, mm_y+495), (xl_x+230, pt_vout_m[1]), pt_vout_m], (15, 23, 42), width=6, label="Jarum Hitam (-) ke OUT-", label_pos=(1160, 660), labels_queue=lbls)

    flush_labels(d, lbls)

    d.rounded_rectangle([35, 725, 1565, 1020], radius=14, fill=(255, 255, 255), outline=(22, 101, 52), width=3)
    d.rectangle([35, 725, 1565, 770], fill=(22, 101, 52))
    d.text((55, 737), "CARA MELAKUKAN STEP 1 DENGAN AMAN:", fill=(255, 255, 255), font=get_font(20, bold=True))
    steps = [
        "1. Rakit 4 dioda 1N4007 dan 4 resistor 10k di PCB bolong sesuai kotak kuning di atas. Pastikan gelang perak dioda menghadap ke KANAN.",
        "2. Hubungkan jalur +48V ke lubang IN+ XL7015, dan jalur GND ke lubang IN- XL7015.",
        "3. Colok kabel LAN dari Switch PoE. Tempelkan jarum multimeter MERAH ke OUT+ dan jarum HITAM ke OUT-.",
        "4. Awalnya layar multimeter akan menampilkan angka tinggi (sekitar 12V - 20V). Ambil obeng minus kecil, putar BAUT KUNINGAN pada kotak biru",
        "   XL7015 ke arah KIRI (berlawanan jarum jam) terus-menerus sampai angka di multimeter turun menjadi TEPAT 5.00 V.",
        "5. Setelah layar multimeter stabil di 5.00 V, cabut kabel LAN! Sekarang modul XL7015 sudah jinak dan siap disambung ke ESP32."
    ]
    for idx, st in enumerate(steps):
        d.text((55, 785 + idx*36), st, fill=(15, 23, 42), font=get_font(16, bold=("5.00 V" in st or "KIRI" in st)))

    save_both(img, "step1_kalibrasi_xl7015.png")


# ==============================================================================
# GAMBAR 2: STEP 2 - WIRING LAYAR OLED 0.91" (SESUAI FOTO IMG_7736) KE ESP32 (IMG_7735)
# ==============================================================================
def make_step2():
    W, H = 1600, 1020
    img = Image.new("RGB", (W, H), (248, 250, 252))
    d = ImageDraw.Draw(img)
    lbls = []

    d.rectangle([0, 0, W, 90], fill=(29, 78, 216))
    d.text((35, 15), "STEP 2: PASANG LAYAR OLED 0.91\" KE ESP32 (SISI KIRI: D25 & D26 - RAPI SEIMBANG)", fill=(255, 255, 255), font=get_font(26, bold=True))
    d.text((35, 53), "Kabel data OLED dipindah ke SISI KIRI (D25 SDA, D26 SCL) agar sisi kanan tidak menumpuk dengan W5500!", fill=(219, 234, 254), font=get_font(17, bold=True))

    esp_coords = draw_esp32(d, 90, 130, active_left=["D25", "D26", "GND_L"], active_right=["3V3"])

    ox, oy = 1020, 220
    ow, oh = 520, 340
    d.rounded_rectangle([ox, oy, ox+ow, oy+oh], radius=16, fill=(15, 60, 120), outline=(30, 41, 59), width=4)
    d.rounded_rectangle([ox+130, oy+35, ox+ow-30, oy+oh-35], radius=8, fill=(10, 15, 25), outline=(56, 189, 248), width=3)
    d.text((ox+165, oy+95), "OLED 0.91\" BIRU", fill=(56, 189, 248), font=get_font(28, bold=True))
    d.text((ox+165, oy+140), "(Sesuai Foto IMG_7736)", fill=(250, 204, 21), font=get_font(19, bold=True))
    d.text((ox+165, oy+180), "I2C SDA: D25 | SCL: D26", fill=(255, 255, 255), font=get_font(16, bold=True))
    d.text((ox+165, oy+205), "Rapi di Sisi Kiri ESP32!", fill=(255, 255, 255), font=get_font(16, bold=True))

    oled_pins = [
        ("SDA", oy+65),
        ("SCK", oy+135),
        ("VCC", oy+205),
        ("GND", oy+275),
    ]
    oled_pts = {}
    for pname, py in oled_pins:
        d.rectangle([ox+15, py-13, ox+45, py+13], fill=(250, 204, 21), outline=(255,255,255), width=2)
        d.text((ox+55, py-12), pname, fill=(255, 255, 255), font=get_font(21, bold=True, mono=True))
        oled_pts[pname] = (ox+15, py)

    # 1. SDA -> D25 (Sisi Kiri No. 8 dari Atas)
    draw_wire(d, [
        oled_pts["SDA"],
        (940, oled_pts["SDA"][1]),
        (940, 105),
        (55, 105),
        (55, esp_coords["D25"][1]),
        esp_coords["D25"]
    ], (16, 185, 129), width=7, label="1. Pin SDA OLED  --->  Pin D25 ESP32 (Sisi Kiri No. 8 dari Atas)", label_pos=(520, 95), labels_queue=lbls)

    # 2. SCK / SCL -> D26 (Sisi Kiri No. 9 dari Atas)
    draw_wire(d, [
        oled_pts["SCK"],
        (900, oled_pts["SCK"][1]),
        (900, 118),
        (45, 118),
        (45, esp_coords["D26"][1]),
        esp_coords["D26"]
    ], (37, 99, 235), width=7, label="2. Pin SCK / SCL OLED  --->  Pin D26 ESP32 (Sisi Kiri No. 9 dari Atas)", label_pos=(520, 140), labels_queue=lbls)

    # 3. VCC -> 3V3 (Kanan Paling Bawah)
    draw_wire(d, [
        oled_pts["VCC"],
        (850, oled_pts["VCC"][1]),
        (850, esp_coords["3V3"][1]),
        esp_coords["3V3"]
    ], (220, 38, 38), width=7, label="3. Pin VCC OLED  --->  Pin 3V3 ESP32 (Kanan PALING BAWAH / No. 15)", label_pos=(520, 622), labels_queue=lbls)

    # 4. GND -> GND_L (Kiri No. 2 dari Bawah)
    draw_wire(d, [
        oled_pts["GND"],
        (800, oled_pts["GND"][1]),
        (800, 920),
        (65, 920),
        (65, esp_coords["GND_L"][1]),
        esp_coords["GND_L"]
    ], (30, 41, 59), width=7, label="4. Pin GND OLED  --->  Pin GND ESP32 (Kiri No. 2 dari Bawah)", label_pos=(520, 905), labels_queue=lbls)

    flush_labels(d, lbls)

    d.rounded_rectangle([520, 685, 1550, 965], radius=14, fill=(240, 253, 244), outline=(22, 101, 52), width=3)
    d.text((545, 705), "SKEMA PIN BARU (SEIMBANG KIRI-KANAN):", fill=(22, 101, 52), font=get_font(20, bold=True))
    tips = [
        "1. KABEL DATA I2C PINDAH KE SISI KIRI: SDA ke D25, SCL ke D26 (GPIO Matrix ESP32).",
        "2. Di kode Arduino: Cukup gunakan 'Wire.begin(25, 26);' (SDA=25, SCL=26).",
        "3. Sisi Kanan sekarang bersih dan khusus disediakan untuk jalur modul Ethernet W5500.",
        "4. Tulisan 'SCK' pada papan OLED Anda sama artinya dengan 'SCL' (Clock I2C)."
    ]
    for i, t in enumerate(tips):
        d.text((545, 745 + i*38), t, fill=(15, 23, 42), font=get_font(16, bold=(i in [0,1])))

    save_both(img, "step2_wiring_oled.png")


# ==============================================================================
# GAMBAR 3: STEP 3 - WIRING SENSOR AS608 (SESUAI FOTO IMG_7733 & IMG_7734!)
# ==============================================================================
def make_step3():
    W, H = 1600, 1080
    img = Image.new("RGB", (W, H), (248, 250, 252))
    d = ImageDraw.Draw(img)
    lbls = []

    d.rectangle([0, 0, W, 95], fill=(185, 28, 28))
    d.text((35, 12), "STEP 3: KABEL SENSOR AS608 KE SISI KIRI ESP32 (D32, D33, D35 - RAPI SEIMBANG)", fill=(255, 255, 255), font=get_font(26, bold=True))
    d.text((35, 50), "Kabel data Sensor Sidik Jari dipindah ke SISI KIRI (D32 RX2, D33 TX2) berdampingan dengan Pin Sentuh D35!", fill=(254, 240, 138), font=get_font(16, bold=True))

    esp_coords = draw_esp32(d, 90, 140, active_left=["D35", "D32", "D33", "GND_L"], active_right=["3V3"])

    fx, fy = 1010, 140
    fw, fh = 550, 510
    d.rounded_rectangle([fx, fy, fx+fw, fy+fh], radius=18, fill=(20, 83, 45), outline=(15, 23, 42), width=4)
    d.text((fx+25, fy+16), "SENSOR AS608 (SOKET 8-PIN DI FOTO IMG_7733)", fill=(250, 204, 21), font=get_font(18, bold=True))
    d.text((fx+25, fy+42), "Ikuti posisi urutan pin & warna kabel asli di foto IMG_7734:", fill=(255, 255, 255), font=get_font(14, bold=True))

    d.rounded_rectangle([fx+20, fy+70, fx+530, fy+490], radius=10, fill=(248, 250, 252), outline=(148, 163, 184), width=3)

    fp_pins = [
        ("P1", "Pin 1 [V+]  -> Kabel COKLAT (+3.3V Utama)",     fy+100, (120, 53, 15)),
        ("P2", "Pin 2 [TX]  -> Kabel ORANYE (Ke D32 ESP32)",    fy+152, (249, 115, 22)),
        ("P3", "Pin 3 [RX]  -> Kabel PUTIH (Ke D33 ESP32)",     fy+204, (255, 255, 255)),
        ("P4", "Pin 4 [GND] -> Kabel BIRU (Ke GND ESP32)",      fy+256, (37, 99, 235)),
        ("P5", "Pin 5 [TCH] -> Kabel HIJAU (Ke D35 ESP32)",     fy+308, (22, 163, 74)),
        ("P6", "Pin 6 [VA]  -> Kabel KUNING (+3.3V Sentuh!)",   fy+360, (202, 138, 4)),
        ("P7", "Pin 7 [D+]  -> Kabel HITAM (JANGAN DIPAKAI!)",  fy+412, (15, 23, 42)),
        ("P8", "Pin 8 [D-]  -> Kabel MERAH (JANGAN DIPAKAI!)",  fy+464, (220, 38, 38)),
    ]
    fp_pts = {}
    for pcode, pdesc, py, col in fp_pins:
        d.rectangle([fx+30, py-12, fx+62, py+12], fill=col, outline=(15, 23, 42), width=2)
        txt_c = (185, 28, 28) if pcode in ["P7", "P8"] else (15, 23, 42)
        d.text((fx+72, py-11), pdesc, fill=txt_c, font=get_font(15, bold=True))
        fp_pts[pcode] = (fx+30, py)

    # 1. Pin 1 (V+ Coklat) & Pin 6 (VA Kuning) digabung -> 3V3 ESP32 (Kanan Paling Bawah!)
    draw_wire(d, [
        fp_pts["P1"],
        (940, fp_pts["P1"][1]),
        (940, fp_pts["P6"][1]),
        fp_pts["P6"]
    ], (202, 138, 4), width=6)
    draw_wire(d, [
        (940, fp_pts["P6"][1]),
        (940, esp_coords["3V3"][1]),
        esp_coords["3V3"]
    ], (220, 38, 38), width=7, label="1. Gabung Kabel COKLAT (V+) & KUNING (VA)  --->  Pin 3V3 ESP32 (Kanan Paling Bawah)", label_pos=(500, 622), labels_queue=lbls)

    # 2. Pin 2 (TX Oranye) -> D32 ESP32 (Sisi Kiri No. 6 dari Atas)
    draw_wire(d, [
        fp_pts["P2"],
        (820, fp_pts["P2"][1]),
        (820, 115),
        (50, 115),
        (50, esp_coords["D32"][1]),
        esp_coords["D32"]
    ], (249, 115, 22), width=6, label="2. Kabel ORANYE (Pin 2 TX)  --->  Pin D32 ESP32 (Sisi Kiri No. 6 dari Atas)", label_pos=(500, 205), labels_queue=lbls)

    # 3. Pin 3 (RX Putih) -> D33 ESP32 (Sisi Kiri No. 7 dari Atas)
    draw_wire(d, [
        fp_pts["P3"],
        (760, fp_pts["P3"][1]),
        (760, 128),
        (40, 128),
        (40, esp_coords["D33"][1]),
        esp_coords["D33"]
    ], (255, 255, 255), width=6, label="3. Kabel PUTIH (Pin 3 RX)  --->  Pin D33 ESP32 (Sisi Kiri No. 7 dari Atas)", label_pos=(500, 250), labels_queue=lbls)

    # 4. Pin 5 (TCH Hijau) -> D35 ESP32 (Sisi Kiri No. 5 dari Atas)
    draw_wire(d, [
        fp_pts["P5"],
        (880, fp_pts["P5"][1]),
        (880, 102),
        (60, 102),
        (60, esp_coords["D35"][1]),
        esp_coords["D35"]
    ], (22, 163, 74), width=6, label="4. Kabel HIJAU (Pin 5 TCH)  --->  Pin D35 ESP32 (Sisi Kiri No. 5 dari Atas)", label_pos=(500, 95), labels_queue=lbls)

    # 5. Pin 4 (GND Biru) -> GND_L ESP32 (Kiri No. 2 dari Bawah)
    draw_wire(d, [
        fp_pts["P4"],
        (640, fp_pts["P4"][1]),
        (640, 935),
        (55, 935),
        (55, esp_coords["GND_L"][1]),
        esp_coords["GND_L"]
    ], (37, 99, 235), width=6, label="5. Kabel BIRU (Pin 4 GND)  --->  Pin GND ESP32 (Kiri No. 2 dari Bawah)", label_pos=(500, 920), labels_queue=lbls)

    # Resistor 10k antara D35 dan GND_L di sisi kiri luar ESP32
    res_y1, res_y2 = esp_coords["D35"][1] + 45, esp_coords["GND_L"][1] - 45
    d.line([(60, esp_coords["D35"][1]), (22, esp_coords["D35"][1]), (22, res_y1)], fill=(202, 138, 4), width=5)
    d.line([(22, res_y2), (22, esp_coords["GND_L"][1]), (55, esp_coords["GND_L"][1])], fill=(15, 23, 42), width=5)
    d.rounded_rectangle([8, res_y1, 36, res_y2], radius=6, fill=(217, 119, 6), outline=(120, 53, 15), width=3)
    draw_badge(d, "Resistor 10k (D35 ke GND)", (500, 150), (217, 119, 6))
    d.line([(36, (res_y1+res_y2)//2), (80, (res_y1+res_y2)//2)], fill=(217, 119, 6), width=3)

    flush_labels(d, lbls)

    d.rounded_rectangle([510, 680, 1560, 890], radius=14, fill=(240, 253, 244), outline=(22, 101, 52), width=3)
    d.text((530, 695), "SKEMA PIN BARU AS608 DI SISI KIRI (BERDAMPINGAN D35, D32, D33):", fill=(22, 101, 52), font=get_font(19, bold=True))
    notes = [
        "1. KABEL DATA PINDAH KE SISI KIRI: TX Oranye ke D32, RX Putih ke D33 (Tepat di bawah Pin Touch D35).",
        "2. Di kode Arduino: Cukup gunakan 'mySerial.begin(57600, SERIAL_8N1, 32, 33);' (RX=32, TX=33).",
        "3. Pin V+ dan VA tetap disatukan ke 3V3 (Kabel Coklat + Kuning), Kabel Biru ke GND.",
        "4. Kabel HITAM (D+) dan MERAH (D-) adalah USB, tetap dikosongkan/jangan disambung."
    ]
    for i, n in enumerate(notes):
        d.text((530, 732 + i*34), n, fill=(15, 23, 42), font=get_font(15, bold=(i in [0,1])))

    save_both(img, "step3_wiring_fingerprint_as608.png")


# ==============================================================================
# GAMBAR 4: STEP 4 - WIRING BUZZER AKTIF + RESISTOR 1K + DIODA 1N4007
# ==============================================================================
def make_step4():
    W, H = 1600, 980
    img = Image.new("RGB", (W, H), (248, 250, 252))
    d = ImageDraw.Draw(img)
    lbls = []

    d.rectangle([0, 0, W, 90], fill=(22, 101, 52))
    d.text((35, 15), "STEP 4: CARA PASANG BUZZER 3.3V, RESISTOR 1K & DIODA KE ESP32", fill=(255, 255, 255), font=get_font(28, bold=True))
    d.text((35, 53), "Menggunakan pin D14 (Kiri No. 5 dari Bawah) dan GND pada ESP32 sesuai foto IMG_7735", fill=(187, 247, 208), font=get_font(18, bold=True))

    esp_coords = draw_esp32(d, 90, 140, active_left=["D14", "GND_L"], active_right=[])

    bx, by = 1190, 250
    d.ellipse([bx, by, bx+260, by+260], fill=(15, 23, 42), outline=(71, 85, 105), width=5)
    d.ellipse([bx+95, by+95, bx+165, by+165], fill=(51, 65, 85), outline=(148, 163, 184), width=2)
    d.text((bx+55, by+35), "ACTIVE BUZZER", fill=(255, 255, 255), font=get_font(20, bold=True))
    d.text((bx+105, by+185), "( + )", fill=(239, 68, 68), font=get_font(26, bold=True))
    d.text((bx+95, by+220), "Bodi Buzzer", fill=(252, 165, 165), font=get_font(14, bold=True))

    pt_buz_pos = (bx, by+80)
    pt_buz_neg = (bx, by+190)
    d.rectangle([bx-20, pt_buz_pos[1]-10, bx+5, pt_buz_pos[1]+10], fill=(250, 204, 21))
    d.rectangle([bx-20, pt_buz_neg[1]-10, bx+5, pt_buz_neg[1]+10], fill=(250, 204, 21))
    d.text((bx+15, pt_buz_pos[1]-12), "KAKI (+) PANJANG", fill=(250, 204, 21), font=get_font(16, bold=True))
    d.text((bx+15, pt_buz_neg[1]-12), "KAKI (-) PENDEK", fill=(203, 213, 225), font=get_font(16, bold=True))

    rx, ry = 740, pt_buz_pos[1]
    d.rounded_rectangle([rx, ry-22, rx+170, ry+22], radius=10, fill=(217, 119, 6), outline=(120, 53, 15), width=3)
    d.text((rx+20, ry-10), "RESISTOR 1K", fill=(255, 255, 255), font=get_font(16, bold=True))

    dx, dy_top, dy_bot = 1040, pt_buz_pos[1], pt_buz_neg[1]
    d.line([(dx, dy_top), (dx, dy_bot)], fill=(100, 116, 139), width=6)
    d.rectangle([dx-22, dy_top+22, dx+22, dy_bot-22], fill=(15, 23, 42), outline=(0,0,0), width=2)
    d.rectangle([dx-22, dy_top+26, dx+22, dy_top+44], fill=(226, 232, 240))
    d.text((dx-205, dy_top+26), "Gelang Perak di Atas (+) ->", fill=(185, 28, 28), font=get_font(13, bold=True))
    d.text((dx-195, dy_bot-38), "Badan Hitam di Bawah (-) ->", fill=(15, 23, 42), font=get_font(13, bold=True))

    draw_wire(d, [
        esp_coords["D14"],
        (35, esp_coords["D14"][1]),
        (35, 905),
        (560, 905),
        (560, ry),
        (rx, ry)
    ], (234, 88, 12), width=7, label="1. Pin D14 ESP32 (Kiri No. 5 dari Bawah / Atasnya D12)  --->  Kaki Kiri Resistor 1k", label_pos=(530, 195), labels_queue=lbls)

    draw_wire(d, [
        (rx+170, ry),
        pt_buz_pos
    ], (220, 38, 38), width=7, label="2. Kaki Kanan Resistor 1k  --->  Kaki (+) Buzzer", label_pos=(860, 255), labels_queue=lbls)

    draw_wire(d, [
        pt_buz_neg,
        (630, pt_buz_neg[1]),
        (630, 940),
        (65, 940),
        (65, esp_coords["GND_L"][1]),
        esp_coords["GND_L"]
    ], (30, 41, 59), width=7, label="3. Kaki (-) Buzzer  --->  Pin GND ESP32 (Kiri No. 2 dari Bawah)", label_pos=(650, 560), labels_queue=lbls)

    flush_labels(d, lbls)

    d.rounded_rectangle([650, 650, 1550, 880], radius=14, fill=(240, 253, 244), outline=(22, 101, 52), width=3)
    d.text((675, 670), "CARA MEMBEDAKAN KAKI (+) DAN (-) BUZZER:", fill=(22, 101, 52), font=get_font(19, bold=True))
    b_notes = [
        "1. PERINGATAN STRAPPING PIN: Resistor Buzzer WAJIB ke D14 (Kiri No. 5 dari Bawah). JANGAN ke D12!",
        "   (Jika D12 dicolok, memori flash ESP32 terkunci dan gagal boot / gagal flash).",
        "2. Kaki (+) Buzzer ke D14 (atau Kolektor Transistor NPN jika buzzer 5V), Kaki (-) ke GND.",
        "3. Jika menggunakan Buzzer 5V: Gunakan transistor NPN (S8050/BC547) karena arus GPIO 3.3V terbatas.",
        "4. Untuk Active Buzzer 3.3V: Bisa langsung disambung D14 -> Kaki (+) dan GND -> Kaki (-).",
        "5. Dioda Flyback: Gelang Perak WAJIB menghadap ke kaki Positif (+). Jika terbalik, arus korslet ke GND!"
    ]
    for i, bn in enumerate(b_notes):
        d.text((675, 710 + i*30), bn, fill=(15, 23, 42), font=get_font(15, bold=(i in [0,3])))

    save_both(img, "step4_wiring_buzzer.png")


# ==============================================================================
# GAMBAR 5: STEP 5 - WIRING MODUL ETHERNET W5500 BIRU (SESUAI FOTO IMG_7737 & IMG_7738)
# ==============================================================================
def make_step5_w5500():
    W, H = 1600, 1060
    img = Image.new("RGB", (W, H), (248, 250, 252))
    d = ImageDraw.Draw(img)
    lbls = []

    d.rectangle([0, 0, W, 95], fill=(30, 58, 138))
    d.text((35, 14), "STEP 5: PASANG MODUL ETHERNET W5500 BIRU (SESUAI FOTO IMG_7737 & IMG_7738)", fill=(255, 255, 255), font=get_font(26, bold=True))
    d.text((35, 52), "Perhatikan 10 pin kuning (2 baris x 5 kolom) di belakang modul W5500 Anda pada foto IMG_7738!", fill=(191, 219, 254), font=get_font(18, bold=True))

    esp_coords = draw_esp32(d, 90, 140, active_left=[], active_right=["D23", "D19", "D18", "D5", "D15", "GND_R", "3V3"])

    wx, wy = 1030, 150
    ww, wh = 530, 490
    d.rounded_rectangle([wx, wy, wx+ww, wy+wh], radius=18, fill=(29, 78, 216), outline=(15, 23, 42), width=4)
    d.text((wx+25, wy+18), "MODUL W5500 BIRU (TAMPAK BELAKANG)", fill=(250, 204, 21), font=get_font(20, bold=True))
    d.text((wx+25, wy+46), "Persis sesuai foto IMG_7738 (Header Kuning 2x5 Pin):", fill=(255, 255, 255), font=get_font(15, bold=True))

    # Gambar kotak konektor RJ45 di bagian atas seperti di IMG_7738
    d.rounded_rectangle([wx+145, wy+80, wx+385, wy+170], radius=8, fill=(203, 213, 225), outline=(71, 85, 105), width=3)
    d.text((wx+170, wy+105), "SOKET LAN RJ45", fill=(15, 23, 42), font=get_font(18, bold=True))
    d.text((wx+165, wy+132), "(HanRun HR911105A)", fill=(51, 65, 85), font=get_font(14, bold=True))

    # Gambar Header Kuning 2x5 diputar vertikal supaya mudah ditarik garis ke kiri,
    # sambil menampilkan posisi Kolom 1-5 Baris Atas & Bawah persis seperti IMG_7738!
    d.rounded_rectangle([wx+20, wy+190, wx+510, wy+470], radius=12, fill=(254, 249, 195), outline=(202, 138, 4), width=3)
    d.text((wx+35, wy+200), "PIN HEADER KUNING 2x5 (Lihat tulisan di foto IMG_7738):", fill=(146, 64, 14), font=get_font(15, bold=True))

    w5500_pins = [
        ("SCLK", "Pin SCLK (Kolom 5 Atas)  -> Ke D18", wy+242, (234, 88, 12)),
        ("SCS",  "Pin SCS  (Kolom 4 Atas)  -> Ke D5",  wy+280, (147, 51, 234)),
        ("MOSI", "Pin MOSI (Kolom 3 Atas)  -> Ke D23", wy+318, (37, 99, 235)),
        ("MISO", "Pin MISO (Kolom 2 Atas)  -> Ke D19", wy+356, (16, 185, 129)),
        ("3V3",  "Pin 3.3V (Kolom 1 Atas)  -> Ke 3V3", wy+394, (220, 38, 38)),
        ("RST",  "Pin RST  (Kolom 3 Bawah) -> Ke 3V3", wy+432, (202, 138, 4)),
        ("GND",  "Pin GND  (Kolom 2/5 Bwh) -> Ke GND", wy+462, (30, 41, 59)),
    ]
    w_pts = {}
    for pcode, pdesc, py, col in w5500_pins:
        d.rectangle([wx+35, py-10, wx+65, py+10], fill=(250, 204, 21), outline=(15, 23, 42), width=2)
        d.text((wx+75, py-10), pdesc, fill=(15, 23, 42), font=get_font(15, bold=True))
        w_pts[pcode] = (wx+35, py)

    # Semua 7 pin W5500 masuk ke deretan KANAN ESP32! Sangat rapi!
    # 1. MOSI -> D23 (Right #1, top)
    draw_wire(d, [
        w_pts["MOSI"],
        (560, w_pts["MOSI"][1]),
        (560, esp_coords["D23"][1]),
        esp_coords["D23"]
    ], (37, 99, 235), width=6, label="1. MOSI W5500  --->  Pin D23 ESP32 (Kanan Paling Atas / No. 1)", label_pos=(510, 155), labels_queue=lbls)

    # 2. MISO -> D19 (Right #6)
    draw_wire(d, [
        w_pts["MISO"],
        (610, w_pts["MISO"][1]),
        (610, esp_coords["D19"][1]),
        esp_coords["D19"]
    ], (16, 185, 129), width=6, label="2. MISO W5500  --->  Pin D19 ESP32 (Kanan No. 6 dari Atas)", label_pos=(510, 345), labels_queue=lbls)

    # 3. SCLK -> D18 (Right #7)
    draw_wire(d, [
        w_pts["SCLK"],
        (660, w_pts["SCLK"][1]),
        (660, esp_coords["D18"][1]),
        esp_coords["D18"]
    ], (234, 88, 12), width=6, label="3. SCLK W5500  --->  Pin D18 ESP32 (Kanan No. 7 dari Atas)", label_pos=(510, 215), labels_queue=lbls)

    # 4. SCS -> D5 (Right #8)
    draw_wire(d, [
        w_pts["SCS"],
        (710, w_pts["SCS"][1]),
        (710, esp_coords["D5"][1]),
        esp_coords["D5"]
    ], (147, 51, 234), width=6, label="4. SCS W5500   --->  Pin D5 ESP32 (Kanan No. 8 dari Atas)", label_pos=(510, 280), labels_queue=lbls)

    # 5. RST -> D15 (Right #13)
    draw_wire(d, [
        w_pts["RST"],
        (760, w_pts["RST"][1]),
        (760, esp_coords["D15"][1]),
        esp_coords["D15"]
    ], (202, 138, 4), width=6, label="5. RST W5500   --->  Pin 3V3 ESP32 (Wakeup Active-High / Normal Operasi)", label_pos=(510, 535), labels_queue=lbls)

    # 6. GND -> GND_R (Right #14)
    draw_wire(d, [
        w_pts["GND"],
        (810, w_pts["GND"][1]),
        (810, esp_coords["GND_R"][1]),
        esp_coords["GND_R"]
    ], (30, 41, 59), width=6, label="6. GND W5500   --->  Pin GND ESP32 (Kanan No. 2 dari Bawah)", label_pos=(510, 588), labels_queue=lbls)

    # 7. 3.3V -> 3V3 (Right #15)
    draw_wire(d, [
        w_pts["3V3"],
        (860, w_pts["3V3"][1]),
        (860, esp_coords["3V3"][1]),
        esp_coords["3V3"]
    ], (220, 38, 38), width=6, label="7. 3.3V W5500  --->  Pin 3V3 ESP32 (Kanan Paling Bawah / No. 15)", label_pos=(510, 640), labels_queue=lbls)

    flush_labels(d, lbls)

    d.rounded_rectangle([510, 695, 1560, 1020], radius=14, fill=(255, 255, 255), outline=(30, 58, 138), width=3)
    d.rectangle([510, 695, 1560, 740], fill=(30, 58, 138))
    d.text((530, 707), "PANDUAN MEMBACA 10 PIN KUNING W5500 DI FOTO IMG_7738 MILIKMU:", fill=(255, 255, 255), font=get_font(18, bold=True))
    w_notes = [
        "1. Pegang modul W5500 menghadap bagian belakang seperti di foto IMG_7738 (pin kuning di bawah):",
        "   - BARIS ATAS (kiri ke kanan)   :  [3.3V]   [MISO]   [MOSI]   [SCS]   [SCLK]",
        "   - BARIS BAWAH (kiri ke kanan) :  [ 5V ]   [GND ]   [RST ]   [INT]   [GND ]",
        "2. Pin 5V dan INT boleh dikosongkan (tidak dipakai) karena kita sudah memberi daya lewat pin 3.3V.",
        "3. CATATAN PoE UNTUK W5500: Jika Anda mencolokkan kabel LAN PoE ke soket RJ45 di modul W5500 ini,",
        "   ambil 4 kabel PoE (Biru, Putih-Biru, Coklat, Putih-Coklat) sebelum masuk konektor RJ45 (atau solder",
        "   dari kaki timah RJ45 nomor 4,5 dan 7,8 di balik W5500) untuk masuk ke Dioda Bridge Step 1!"
    ]
    for i, wn in enumerate(w_notes):
        d.text((530, 752 + i*35), wn, fill=(15, 23, 42), font=get_font(15, bold=(i in [1,2,4])))

    save_both(img, "step5_wiring_w5500.png")


# ==============================================================================
# GAMBAR 6: STEP 6 (TERAKHIR) - SAMBUNGAN DAYA 5.00V XL7015 KE PIN VIN & GND ESP32
# ==============================================================================
def make_step6_power():
    W, H = 1600, 980
    img = Image.new("RGB", (W, H), (248, 250, 252))
    d = ImageDraw.Draw(img)
    lbls = []

    d.rectangle([0, 0, W, 90], fill=(15, 23, 42))
    d.text((35, 15), "STEP 6 (TERAKHIR): MENYAMBUNGKAN DAYA 5.00V XL7015 KE ESP32", fill=(250, 204, 21), font=get_font(28, bold=True))
    d.text((35, 53), "HANYA lakukan tahap ini SETELAH Step 1 selesai dan multimeter sudah membuktikan output XL7015 = 5.00V!", fill=(255, 255, 255), font=get_font(18, bold=True))

    esp_coords = draw_esp32(d, 90, 140, active_left=["VIN", "GND_L"], active_right=[])

    xl_x, xl_y = 1090, 220
    d.rounded_rectangle([xl_x, xl_y, xl_x+410, xl_y+350], radius=16, fill=(22, 101, 52), outline=(20, 83, 45), width=4)
    d.text((xl_x+35, xl_y+25), "MODUL STEP-DOWN XL7015", fill=(255, 255, 255), font=get_font(22, bold=True))
    d.text((xl_x+35, xl_y+58), "(SUDAH DISETEL 5.00V DC)", fill=(250, 204, 21), font=get_font(18, bold=True))

    pt_out_p = (xl_x+30, xl_y+140)
    pt_out_m = (xl_x+30, xl_y+265)
    d.ellipse([pt_out_p[0]-18, pt_out_p[1]-18, pt_out_p[0]+18, pt_out_p[1]+18], fill=(250, 204, 21), outline=(255,255,255), width=3)
    d.ellipse([pt_out_m[0]-18, pt_out_m[1]-18, pt_out_m[0]+18, pt_out_m[1]+18], fill=(250, 204, 21), outline=(255,255,255), width=3)
    d.text((pt_out_p[0]+30, pt_out_p[1]-12), "OUT+ (+5.00V)", fill=(255, 255, 255), font=get_font(20, bold=True))
    d.text((pt_out_m[0]+30, pt_out_m[1]-12), "OUT- (GND)", fill=(255, 255, 255), font=get_font(20, bold=True))

    d5_x, d5_y = 750, pt_out_p[1]
    d.rectangle([d5_x, d5_y-22, d5_x+130, d5_y+22], fill=(15, 23, 42), outline=(0,0,0), width=3)
    d.rectangle([d5_x+8, d5_y-22, d5_x+32, d5_y+22], fill=(226, 232, 240))
    d.text((d5_x+42, d5_y-10), "DIODA D5", fill=(255, 255, 255), font=get_font(15, bold=True))
    d.text((d5_x-30, d5_y-58), "Perhatikan Arah Gelang Perak D5:", fill=(185, 28, 28), font=get_font(16, bold=True))
    d.text((d5_x-30, d5_y-36), "<--- Gelang Perak menghadap ke KIRI (ke VIN ESP32)", fill=(185, 28, 28), font=get_font(15, bold=True))

    draw_wire(d, [pt_out_p, (d5_x+130, d5_y)], (220, 38, 38), width=8)
    draw_wire(d, [
        (d5_x, d5_y),
        (550, d5_y),
        (550, 935),
        (35, 935),
        (35, esp_coords["VIN"][1]),
        esp_coords["VIN"]
    ], (220, 38, 38), width=8, label="JALUR POSITIF (+5V): Lubang OUT+  --->  Dioda D5  --->  Pin VIN ESP32 (Kiri Paling Bawah)", label_pos=(560, 918), labels_queue=lbls)

    draw_wire(d, [
        pt_out_m,
        (615, pt_out_m[1]),
        (615, 895),
        (65, 895),
        (65, esp_coords["GND_L"][1]),
        esp_coords["GND_L"]
    ], (30, 41, 59), width=8, label="JALUR NEGATIF (GND): Lubang OUT-  --->  Pin GND ESP32 (Kiri No. 2 dari Bawah)", label_pos=(640, 560), labels_queue=lbls)

    flush_labels(d, lbls)

    d.rounded_rectangle([640, 650, 1540, 860], radius=14, fill=(254, 226, 226), outline=(220, 38, 38), width=3)
    d.text((665, 670), "POSISI PIN VIN & GND SESUAI FOTO IMG_7735 MILIKMU:", fill=(185, 28, 28), font=get_font(19, bold=True))
    d.text((665, 710), "- Di foto IMG_7735, pin VIN berada di pojok KIRI PALING BAWAH (sebelah kiri tombol EN).", fill=(15, 23, 42), font=get_font(16, bold=True))
    d.text((665, 742), "- Pin GND berada tepat 1 kaki di atas pin VIN (Kiri No. 2 dari Bawah).", fill=(15, 23, 42), font=get_font(16, bold=True))
    d.text((665, 774), "- Dioda D5 menjaga agar saat Anda colok USB-C ke laptop, arus dari XL7015 dan laptop", fill=(15, 23, 42), font=get_font(16))
    d.text((665, 804), "  tidak saling bertabrakan.", fill=(15, 23, 42), font=get_font(16))

    save_both(img, "step5_wiring_power_lengkap.png")
    save_both(img, "step6_wiring_power_lengkap.png")

if __name__ == "__main__":
    make_step1()
    make_step2()
    make_step3()
    make_step4()
    make_step5_w5500()
    make_step6_power()
    print("ALL 6 REAL-PHOTO MATCHED IMAGES GENERATED!")
