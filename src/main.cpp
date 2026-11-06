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
// INSTANCE OBJEK
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

HardwareSerial fpSerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fpSerial);

// MAC & Default IP dummy untuk deteksi awal hardware W5500
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 199);

bool oledOk = false;
bool fpOk = false;
bool w5500Ok = false;

void updateOLED(const char* statusMsg = "") {
  if (!oledOk) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Header
  display.setTextSize(1);
  display.setCursor(10, 0);
  display.println("ABSENSI POE - ESP32");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  
  // Status Baris 1: OLED & Fingerprint
  display.setCursor(0, 14);
  display.printf("OLED : %s", oledOk ? "OK (0x3C)" : "ERR");
  display.setCursor(0, 24);
  display.printf("AS608: %s (%d fp)", fpOk ? "OK" : "ERR", fpOk ? finger.capacity : 0);
  
  // Status Baris 2: W5500 & LAN Cable
  display.setCursor(0, 34);
  if (w5500Ok) {
    auto link = Ethernet.linkStatus();
    display.printf("W5500: OK (%s)", link == LinkON ? "LAN ON" : "NO LAN");
  } else {
    display.println("W5500: TIDAK DETEK");
  }
  
  // Area Bawah: Notifikasi Sentuhan Jari
  display.drawLine(0, 46, 127, 46, SSD1306_WHITE);
  display.setCursor(0, 50);
  if (statusMsg && strlen(statusMsg) > 0) {
    display.println(statusMsg);
  } else {
    display.println("Tempelkan Jari...");
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("  SISTEM ABSENSI POE - UJI COBA HARDWARE");
  Serial.println("  Layout Simetris 7 Kiri vs 7 Kanan");
  Serial.println("========================================");

  // 1. Inisialisasi OLED (I2C)
  Serial.println("\n[1] Menginisialisasi OLED (SSD1306)...");
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  delay(100);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledOk = true;
    Serial.println(" -> OLED SSD1306 Berhasil Terhubung (0x3C)!");
  } else if (display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    oledOk = true;
    Serial.println(" -> OLED SSD1306 Berhasil Terhubung (0x3D)!");
  } else {
    Serial.println(" -> GAGAL inisialisasi OLED SSD1306.");
  }

  // Tampilkan splash awal di OLED
  if (oledOk) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 15);
    display.println("  MEMULAI PERANGKAT...");
    display.setCursor(0, 30);
    display.println("  Cek sensor & LAN...");
    display.display();
  }

  // 2. Inisialisasi Sensor Sidik Jari AS608
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
    Serial.printf("    - Tingkat Keamanan  : %d\n", finger.security_level);
  } else {
    Serial.println(" -> Sensor AS608 TIDAK merespon.");
  }

  // 3. Inisialisasi Ethernet W5500 (SPI)
  Serial.println("\n[3] Menginisialisasi Ethernet W5500...");
  pinMode(PIN_W5500_RST, OUTPUT);
  // Hardware reset pulse untuk chip W5500
  digitalWrite(PIN_W5500_RST, LOW);
  delay(50);
  digitalWrite(PIN_W5500_RST, HIGH);
  delay(200);

  SPI.begin(PIN_W5500_SCK, PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CS);
  Ethernet.init(PIN_W5500_CS);

  // Inisialisasi controller Ethernet
  Ethernet.begin(mac, ip);

  auto hwStatus = Ethernet.hardwareStatus();
  if (hwStatus == EthernetNoHardware) {
    Serial.println(" -> Modul W5500 TIDAK TERDETEKSI (SPI No Hardware).");
    Serial.println("    Periksa: CS=D27, MOSI=D26, MISO=D25, SCK=D16, RST=D17, 3.3V & GND.");
  } else {
    w5500Ok = true;
    Serial.printf(" -> W5500 Terdeteksi Sempurna! (Tipe chip: %d)\n", (int)hwStatus);
    auto link = Ethernet.linkStatus();
    if (link == LinkON) {
      Serial.println("    - Kabel LAN: Terhubung (Link ON)");
    } else {
      Serial.println("    - Kabel LAN: Belum dicolok (Link OFF)");
    }
  }

  // Update tampilan utama ke Layar OLED
  updateOLED("Menunggu Jari...");

  Serial.println("\n========================================");
  Serial.println("Setup Selesai! Layar OLED telah aktif.");
  Serial.println("Tempelkan jari Anda ke sensor AS608 untuk pengujian!");
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
      updateOLED(">> JARI TERDETEKSI! <<");
      // Coba ambil citra sidik jari
      uint8_t p = finger.getImage();
      if (p == FINGERPRINT_OK) {
        Serial.println(" -> Citra sidik jari berhasil difoto sensor!");
        updateOLED("Foto Sidik: BERHASIL");
      }
    } else {
      Serial.println("[TOUCH] Jari diangkat.");
      updateOLED("Menunggu Jari...");
    }
  }

  // Heartbeat berkala di serial monitor
  if (millis() - lastHeartbeat >= 3000) {
    lastHeartbeat = millis();
    Serial.printf("[HEARTBEAT] Up: %lu s | OLED: %s | FP: %s | W5500: %s | LAN: %s\n",
                  millis() / 1000,
                  oledOk ? "OK" : "ERR",
                  fpOk ? "OK" : "ERR",
                  w5500Ok ? "OK" : "ERR",
                  (w5500Ok && Ethernet.linkStatus() == LinkON) ? "LINK ON" : "NO LINK");
  }

  delay(30);
}
