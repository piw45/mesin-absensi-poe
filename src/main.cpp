#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Fingerprint.h>
#include <SPI.h>
#include <Ethernet.h>

// ==========================================
// PINOUT SIMETRIS 7 KIRI vs 7 KANAN
// ==========================================
// Sisi Kiri (Bawah ke Atas):
// 1. VIN  -> +5V
// 2. GND  -> Ground
// 3. D13  -> OLED SDA
// 4. D12  -> (KOSONG / STRAPPING PIN)
// 5. D14  -> OLED SCL
// 6. D27  -> W5500 SCS / CS
// 7. D26  -> W5500 MOSI
// 8. D25  -> W5500 MISO
#define PIN_OLED_SDA   13
#define PIN_OLED_SCL   14
#define PIN_W5500_CS   27
#define PIN_W5500_MOSI 26
#define PIN_W5500_MISO 25

// Sisi Kanan (Bawah ke Atas):
// 1. 3V3  -> 3.3V Common
// 2. GND  -> Ground
// 3. D15  -> AS608 TX (Orange) -> RX2 ESP32
// 4. D2   -> AS608 RX (Putih)  -> TX2 ESP32
// 5. D4   -> AS608 TCH (Hijau) -> Touch Sense
// 6. D16  -> W5500 SCLK
// 7. D17  -> W5500 RST
#define PIN_FP_RX      15
#define PIN_FP_TX      2
#define PIN_FP_TCH     4
#define PIN_W5500_SCK  16
#define PIN_W5500_RST  17

// ==========================================
// OLED 0.91 INCH (128 x 32 PIXEL)
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32  // Layar tipis memanjang 0.91 inch
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

HardwareSerial fpSerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fpSerial);

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 199);

bool oledOk = false;
bool fpOk = false;
bool w5500Ok = false;

// Tampilan OLED untuk Layar 128x32 (32 piksel = 4 baris teks font 1 atau 2 baris font 2)
void drawOledStandby() {
  if (!oledOk) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Baris 1 (y=0): Header terbalik (highlight bar)
  display.fillRect(0, 0, 128, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(4, 1);
  display.print("ABSENSI POE ESP32");

  // Baris 2 (y=12): Status Hardware
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 12);
  bool lanOn = (w5500Ok && Ethernet.linkStatus() == LinkON);
  display.printf("FP:%s  LAN:%s", fpOk ? "OK" : "ERR", lanOn ? "ON" : "OFF");

  // Baris 3 (y=23): Instruksi
  display.setCursor(0, 23);
  display.print(">> TEMPELKAN JARI <<");

  display.display();
}

void drawOledFingerTouched(bool photoSuccess) {
  if (!oledOk) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (photoSuccess) {
    // Tampilan teks besar saat jari terbaca
    display.setTextSize(2);
    display.setCursor(8, 2);
    display.print("JARI OK!");
    display.setTextSize(1);
    display.setCursor(8, 22);
    display.print("Sidik Foto: BERHASIL");
  } else {
    display.setTextSize(2);
    display.setCursor(4, 8);
    display.print("TERDETEKSI");
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("  SISTEM ABSENSI POE - UJI COBA HARDWARE");
  Serial.println("  Konfigurasi OLED 0.91\" (128x32)");
  Serial.println("========================================");

  // 1. Inisialisasi OLED (128x32)
  Serial.println("\n[1] Menginisialisasi OLED 0.91\" (128x32)...");
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  delay(100);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledOk = true;
    Serial.println(" -> OLED 128x32 Berhasil Aktif (0x3C)!");
  } else if (display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    oledOk = true;
    Serial.println(" -> OLED 128x32 Berhasil Aktif (0x3D)!");
  } else {
    Serial.println(" -> OLED GAGAL inisialisasi.");
  }

  // Tampilkan splash awal
  if (oledOk) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 8);
    display.print("MEMULAI...");
    display.display();
  }

  // 2. Inisialisasi AS608 Fingerprint
  Serial.println("\n[2] Menginisialisasi AS608 Fingerprint...");
  pinMode(PIN_FP_TCH, INPUT);
  fpSerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);
  finger.begin(57600);
  delay(200);

  if (finger.verifyPassword()) {
    fpOk = true;
    Serial.println(" -> Sensor Sidik Jari AS608 TERHUBUNG & OK!");
    finger.getParameters();
    Serial.printf("    - Kapasitas Template: %d\n", finger.capacity);
  } else {
    Serial.println(" -> Sensor AS608 TIDAK merespon.");
  }

  // 3. Inisialisasi Ethernet W5500
  Serial.println("\n[3] Menginisialisasi Ethernet W5500...");
  pinMode(PIN_W5500_RST, OUTPUT);
  digitalWrite(PIN_W5500_RST, LOW);
  delay(50);
  digitalWrite(PIN_W5500_RST, HIGH);
  delay(200);

  SPI.begin(PIN_W5500_SCK, PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CS);
  Ethernet.init(PIN_W5500_CS);
  Ethernet.begin(mac, ip);

  if (Ethernet.hardwareStatus() != EthernetNoHardware) {
    w5500Ok = true;
    Serial.println(" -> Modul W5500 Terdeteksi OK!");
  } else {
    Serial.println(" -> Modul W5500 Tidak Terdeteksi.");
  }

  // Tampilkan UI Standby di Layar 128x32
  drawOledStandby();

  Serial.println("\n========================================");
  Serial.println("Setup Selesai! Layar 0.91\" siap menampilkan teks.");
  Serial.println("========================================\n");
}

int lastTouchState = -1;
unsigned long lastHeartbeat = 0;

void loop() {
  // Cek pin sentuh AS608 (TCH - GPIO 4)
  int touch = digitalRead(PIN_FP_TCH);
  if (touch != lastTouchState) {
    lastTouchState = touch;
    if (touch == HIGH) {
      Serial.println("[TOUCH] Jari terdeteksi menyentuh sensor!");
      drawOledFingerTouched(false);

      // Coba capture gambar sidik
      uint8_t p = finger.getImage();
      if (p == FINGERPRINT_OK) {
        Serial.println(" -> Citra sidik jari berhasil difoto!");
        drawOledFingerTouched(true);
      }
    } else {
      Serial.println("[TOUCH] Jari diangkat.");
      drawOledStandby();
    }
  }

  // Heartbeat setiap 3 detik
  if (millis() - lastHeartbeat >= 3000) {
    lastHeartbeat = millis();
    Serial.printf("[HEARTBEAT] Up: %lu s | OLED: %s | FP: %s | W5500: %s | LAN: %s\n",
                  millis() / 1000,
                  oledOk ? "OK" : "ERR",
                  fpOk ? "OK" : "ERR",
                  w5500Ok ? "OK" : "ERR",
                  (w5500Ok && Ethernet.linkStatus() == LinkON) ? "LINK ON" : "NO LINK");
    // Refresh status link LAN di standby jika tidak sedang disentuh
    if (digitalRead(PIN_FP_TCH) == LOW) {
      drawOledStandby();
    }
  }

  delay(30);
}
