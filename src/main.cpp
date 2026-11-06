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
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

HardwareSerial fpSerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fpSerial);

// Konfigurasi Jaringan & Server Denanyar
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress staticIp(192, 168, 1, 199);
EthernetServer server(80);

const char* serverHost = "denanyar.ac.id";
const int serverPort = 80;

bool oledOk = false;
bool fpOk = false;
bool w5500Ok = false;
bool lanConnected = false;

// Teks tampilan utama yang dapat dikontrol Admin (1 baris besar)
String adminMessage = "TEMPEL JARI";
int lastScannedID = -1;

// Forward declaration
bool enrollFingerprint(int id);

// ==========================================
// TAMPILAN OLED 1 BARIS HURUF BESAR
// ==========================================
void showBigText(String text) {
  if (!oledOk) return;
  text.trim();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  int len = text.length();
  // Gunakan Font Size 2 (Tinggi 16px, lebar 12px) agar huruf besar & tegas
  int size = (len <= 10) ? 2 : 1;
  int charW = size * 6;
  int charH = size * 8;

  // Posisikan tepat di tengah layar secara horizontal dan vertikal
  int x = (SCREEN_WIDTH - (len * charW)) / 2;
  if (x < 0) x = 0;
  int y = (SCREEN_HEIGHT - charH) / 2;

  display.setTextSize(size);
  display.setCursor(x, y);
  display.print(text);
  display.display();
}

// ==========================================
// KOMUNIKASI OTOMATIS KE SERVER DENANYAR
// ==========================================
void syncWithServerDenanyar(const char* action, int fingerId) {
  if (!lanConnected || Ethernet.linkStatus() != LinkON) return;

  EthernetClient client;
  client.setTimeout(2500);
  Serial.printf("\n[SERVER] Menghubungi %s:%d (Aksi: %s)...\n", serverHost, serverPort, action);

  if (client.connect(serverHost, serverPort)) {
    String url = "/admin/api-mesin.php?action=" + String(action);
    url += "&mac=DEADBEEFFEED";
    url += "&ip=" + Ethernet.localIP().toString();
    if (fingerId > 0) {
      url += "&finger_id=" + String(fingerId);
    }

    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + serverHost + "\r\n" +
                 "User-Agent: MesinAbsensiPoE/1.0\r\n" +
                 "Connection: close\r\n\r\n");

    unsigned long startWait = millis();
    String responseLine = "";
    bool isBody = false;

    while (client.connected() && millis() - startWait < 2000) {
      while (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) {
          isBody = true;
          continue;
        }
        if (isBody) {
          responseLine = line;
          break;
        }
      }
      if (responseLine.length() > 0) break;
    }
    client.stop();

    Serial.println(" -> Respon Server: " + responseLine);

    // Jika server memberikan perintah ganti pesan tampilan (format MSG:TEKS)
    int idx = responseLine.indexOf("MSG:");
    if (idx >= 0) {
      String newMsg = responseLine.substring(idx + 4);
      int spaceIdx = newMsg.indexOf(' ');
      if (spaceIdx > 0) newMsg = newMsg.substring(0, spaceIdx);
      int endIdx = newMsg.indexOf('"');
      if (endIdx > 0) newMsg = newMsg.substring(0, endIdx);
      newMsg.trim();
      if (newMsg.length() > 0 && newMsg.length() <= 10 && newMsg != adminMessage) {
        adminMessage = newMsg;
        showBigText(adminMessage);
        Serial.printf("[SERVER] Teks layar diperbarui oleh server: \"%s\"\n", adminMessage.c_str());
      }
    }

    // Jika server mengirimkan perintah khusus (format CMD:ENROLL:slot:mhs_id)
    int cmdIdx = responseLine.indexOf("CMD:ENROLL:");
    if (cmdIdx >= 0) {
      String cmdData = responseLine.substring(cmdIdx + 11);
      int endCmd = cmdData.indexOf('"');
      if (endCmd > 0) cmdData = cmdData.substring(0, endCmd);
      cmdData.trim();

      int sep = cmdData.indexOf(':');
      int enrollSlot = (sep > 0) ? cmdData.substring(0, sep).toInt() : cmdData.toInt();
      int mhsId = (sep > 0) ? cmdData.substring(sep + 1).toInt() : 0;

      if (enrollSlot > 0 && enrollSlot <= 300) {
        Serial.printf("[SERVER CMD] Perintah pendaftaran jari dari Admin SIAKAD! Slot #%d (Mhs #%d)\n", enrollSlot, mhsId);
        bool ok = enrollFingerprint(enrollSlot);
        if (ok) {
          // Lapor balik ke server denanyar
          String doneAction = "enroll_done&finger_id=" + String(enrollSlot) + "&mahasiswa_id=" + String(mhsId);
          syncWithServerDenanyar(doneAction.c_str(), 0);
        }
      }
    }
  } else {
    Serial.println(" -> Server denanyar belum merespon atau offline.");
  }
}

// ==========================================
// KONTROL LAN: KONEKSI OTOMATIS SAAT DICOLOK
// ==========================================
void checkLanConnection() {
  if (!w5500Ok) return;

  bool physicalLink = (Ethernet.linkStatus() == LinkON);

  // 1. Kabel baru saja dicolok
  if (physicalLink && !lanConnected) {
    Serial.println("\n[LAN] Kabel LAN dicolok! Mengambil konfigurasi IP...");
    showBigText("CONNECTING");

    // Otomatis minta DHCP, jika gagal gunakan IP Statis fallback
    if (Ethernet.begin(mac) == 0) {
      Serial.println(" -> DHCP timeout, menggunakan IP Statis 192.168.1.199");
      Ethernet.begin(mac, staticIp);
    }

    lanConnected = true;
    server.begin();
    Serial.printf(" -> Terhubung! IP ESP32: %s\n", Ethernet.localIP().toString().c_str());

    showBigText("ONLINE");
    delay(800);
    showBigText(adminMessage);

    // Otomatis handshake ke server denanyar
    syncWithServerDenanyar("ping", 0);
  }
  // 2. Kabel dicabut
  else if (!physicalLink && lanConnected) {
    lanConnected = false;
    Serial.println("\n[LAN] Kabel LAN dicabut (Link Down).");
    showBigText("OFFLINE");
    delay(800);
    showBigText(adminMessage);
  }
}

// ==========================================
// WEB SERVER KONTROL ADMIN (PORT 80)
// ==========================================
String urlDecode(String str) {
  String decoded = "";
  char a, b;
  for (unsigned int i = 0; i < str.length(); i++) {
    if (str[i] == '+') {
      decoded += ' ';
    } else if (str[i] == '%' && i + 2 < str.length()) {
      a = str[i + 1];
      b = str[i + 2];
      auto hexVal = [](char c) -> int {
        return (c >= '0' && c <= '9') ? (c - '0') :
               (c >= 'A' && c <= 'F') ? (c - 'A' + 10) :
               (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0;
      };
      decoded += (char)(hexVal(a) * 16 + hexVal(b));
      i += 2;
    } else {
      decoded += str[i];
    }
  }
  return decoded;
}

// ==========================================
// PENDAFTARAN SIDIK JARI BARU (ENROLL)
// ==========================================
bool enrollFingerprint(int id) {
  if (!fpOk || id <= 0 || id > 300) return false;
  Serial.printf("\n[ENROLL] Memulai pendaftaran sidik jari untuk Slot ID #%d...\n", id);
  finger.LEDcontrol(true);
  delay(60);
  showBigText("TEMPEL 1");

  // Langkah 1: Tempelkan jari pertama (timeout 12 detik)
  unsigned long start = millis();
  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (millis() - start > 12000) {
      finger.LEDcontrol(false);
      showBigText("TIMEOUT");
      delay(1500);
      showBigText(adminMessage);
      return false;
    }
    delay(40);
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    finger.LEDcontrol(false);
    showBigText("GAGAL 1");
    delay(1500);
    showBigText(adminMessage);
    return false;
  }

  // Langkah 2: Angkat jari
  showBigText("ANGKAT");
  delay(1000);
  while (finger.getImage() != FINGERPRINT_NOFINGER) {
    delay(40);
  }

  // Langkah 3: Tempelkan jari yang sama sekali lagi untuk konfirmasi
  showBigText("TEMPEL 2");
  start = millis();
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (millis() - start > 12000) {
      finger.LEDcontrol(false);
      showBigText("TIMEOUT");
      delay(1500);
      showBigText(adminMessage);
      return false;
    }
    delay(40);
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    finger.LEDcontrol(false);
    showBigText("GAGAL 2");
    delay(1500);
    showBigText(adminMessage);
    return false;
  }

  // Langkah 4: Cocokkan kedua foto sidik
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    finger.LEDcontrol(false);
    showBigText("BEDA JARI");
    delay(1500);
    showBigText(adminMessage);
    return false;
  }

  // Langkah 5: Simpan ke slot memori modul AS608
  p = finger.storeModel(id);
  finger.LEDcontrol(false);
  if (p == FINGERPRINT_OK) {
    Serial.printf(" -> SUKSES! Sidik jari berhasil disimpan ke Slot ID #%d.\n", id);
    showBigText("ID #" + String(id) + " OK");
    delay(2000);
    showBigText(adminMessage);
    return true;
  } else {
    showBigText("GAGAL SIMPAN");
    delay(1500);
    showBigText(adminMessage);
    return false;
  }
}

void handleAdminWebServer() {
  if (!lanConnected) return;

  EthernetClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  client.flush();

  String actionMsg = "";

  // Parsing aksi /enroll?id=X (Tambah Sidik Jari dari Admin)
  if (req.indexOf("GET /enroll?id=") >= 0) {
    int start = req.indexOf("id=") + 3;
    int end = req.indexOf(" ", start);
    if (end == -1) end = req.length();
    int enrollId = req.substring(start, end).toInt();
    if (enrollId > 0 && enrollId <= 300) {
      bool ok = enrollFingerprint(enrollId);
      actionMsg = ok ? "Berhasil mendaftarkan ID #" + String(enrollId) : "Gagal mendaftarkan ID #" + String(enrollId);
    }
  }
  // Parsing aksi /set?text=PESAN
  else if (req.indexOf("GET /set?text=") >= 0) {
    int start = req.indexOf("text=") + 5;
    int end = req.indexOf(" ", start);
    if (end == -1) end = req.length();
    String newText = req.substring(start, end);
    newText = urlDecode(newText);
    newText.trim();
    newText.toUpperCase();
    if (newText.length() > 10) newText = newText.substring(0, 10);
    if (newText.length() > 0) {
      adminMessage = newText;
      showBigText(adminMessage);
      Serial.printf("[ADMIN] Layar diubah ke: \"%s\"\n", adminMessage.c_str());
      actionMsg = "Teks layar diubah ke: " + adminMessage;
    }
  }

  // Kirim halaman Web Dashboard Kontrol Admin
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
  client.println("<title>Kontrol Mesin Absensi Denanyar</title>");
  client.println("<style>");
  client.println("body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;max-width:440px;margin:20px auto;padding:16px;background:#f8fafc;color:#0f172a}");
  client.println(".card{background:#fff;padding:18px;border-radius:14px;box-shadow:0 4px 12px rgba(0,0,0,0.06);margin-bottom:14px;border:1px solid #e2e8f0}");
  client.println("h2{margin:0 0 10px;font-size:18px;color:#047857}p{margin:6px 0;font-size:14px;color:#475569}");
  client.println("input{width:100%;padding:10px;font-size:15px;border:1.5px solid #cbd5e1;border-radius:8px;box-sizing:border-box;margin-top:6px}");
  client.println("button{width:100%;padding:12px;margin-top:10px;background:#059669;color:#fff;border:none;border-radius:8px;font-size:15px;font-weight:bold;cursor:pointer}");
  client.println(".grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:10px}");
  client.println(".qbtn{padding:10px;background:#0284c7;color:#fff;border-radius:8px;text-align:center;text-decoration:none;font-weight:bold;font-size:13px;display:block}");
  client.println(".badge{background:#dcfce7;color:#166534;padding:3px 8px;border-radius:6px;font-size:12px;font-weight:bold}");
  client.println(".alert{background:#e0f2fe;color:#0369a1;padding:10px;border-radius:8px;margin-bottom:12px;font-size:14px;font-weight:bold}");
  client.println("</style></head><body>");

  if (actionMsg.length() > 0) {
    client.printf("<div class='alert'>%s</div>", actionMsg.c_str());
  }

  client.println("<div class='card'>");
  client.println("<h2>Mesin Absensi PoE Denanyar</h2>");
  client.printf("<p>Status LAN: <span class='badge'>ONLINE</span> &bull; IP: <b>%s</b></p>", Ethernet.localIP().toString().c_str());
  client.printf("<p>Sensor FP: <b>%s</b> &bull; Terakhir: <b>ID #%d</b></p>", fpOk ? "OK" : "ERR", lastScannedID);
  client.printf("<p>Teks Layar Aktif: <b style='color:#059669;font-size:16px'>%s</b></p>", adminMessage.c_str());
  client.println("</div>");

  // Form Tambah Sidik Jari (Enrollment)
  client.println("<div class='card'>");
  client.println("<h3 style='margin:0 0 6px;font-size:15px;color:#1e293b'>+ Tambah Sidik Jari (Enroll)</h3>");
  client.println("<form action='/enroll' method='GET'>");
  client.println("<label style='font-size:13px;color:#64748b'>Masukkan Slot ID Mahasiswa (1-300):</label>");
  client.println("<input type='number' name='id' min='1' max='300' placeholder='Contoh: 1' required>");
  client.println("<button type='submit' style='background:#2563eb'>MULAI DAFTARKAN JARI</button>");
  client.println("</form>");
  client.println("</div>");

  // Form Kontrol Teks Layar
  client.println("<div class='card'>");
  client.println("<h3 style='margin:0 0 6px;font-size:15px;color:#1e293b'>Kontrol Layar (1 Baris Besar)</h3>");
  client.println("<form action='/set' method='GET'>");
  client.printf("<input type='text' name='text' maxlength='10' value='%s' placeholder='MAKS 10 HURUF' style='text-transform:uppercase'>", adminMessage.c_str());
  client.println("<button type='submit'>KIRIM KE LAYAR</button>");
  client.println("</form>");
  client.println("<div class='grid'>");
  client.println("<a href='/set?text=TEMPEL+JARI' class='qbtn'>TEMPEL JARI</a>");
  client.println("<a href='/set?text=DENANYAR' class='qbtn'>DENANYAR</a>");
  client.println("<a href='/set?text=UJIAN' class='qbtn' style='background:#d97706'>UJIAN</a>");
  client.println("<a href='/set?text=TUTUP' class='qbtn' style='background:#dc2626'>TUTUP</a>");
  client.println("</div>");
  client.println("</div>");

  client.println("</body></html>");
  delay(10);
  client.stop();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("  SISTEM ABSENSI POE - ESP32 DENANYAR");
  Serial.println("  Tampilan 1 Baris Huruf Besar (0.91\" OLED)");
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

  // Tampilkan splash awal 1 baris besar
  if (oledOk) {
    showBigText("MEMULAI");
  }

  // 2. Inisialisasi AS608 Fingerprint
  Serial.println("\n[2] Menginisialisasi AS608 Fingerprint...");
  pinMode(PIN_FP_TCH, INPUT_PULLDOWN);
  fpSerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);
  finger.begin(57600);
  delay(200);

  if (finger.verifyPassword()) {
    fpOk = true;
    Serial.println(" -> Sensor Sidik Jari AS608 TERHUBUNG & OK!");
    finger.getParameters();
    Serial.printf("    - Kapasitas Template: %d\n", finger.capacity);
    finger.LEDcontrol(false); // Standby: matikan lampu biru sampai disentuh
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

  if (Ethernet.hardwareStatus() != EthernetNoHardware) {
    w5500Ok = true;
    Serial.println(" -> Modul W5500 Terdeteksi OK!");
  } else {
    Serial.println(" -> Modul W5500 Tidak Terdeteksi.");
  }

  // Cek apakah LAN sudah dicolok dari awal
  checkLanConnection();

  // Tampilkan layar standby 1 baris besar
  showBigText(adminMessage);

  Serial.println("\n========================================");
  Serial.println("Setup Selesai! Layar siap dalam mode 1 baris.");
  Serial.println("========================================\n");
}

bool fingerHolding = false;
unsigned long lastFpCheck = 0;
unsigned long lastLanPoll = 0;
unsigned long lastHeartbeat = 0;

void loop() {
  unsigned long now = millis();

  // 1. Cek status fisik koneksi LAN setiap 1 detik
  if (now - lastLanPoll >= 1000) {
    lastLanPoll = now;
    checkLanConnection();
  }

  // 2. Layani Web Server Kontrol Admin jika ada request
  handleAdminWebServer();

  // 3. Mode Standby Sentuh (Pin TCH / GPIO 4)
  int touch = digitalRead(PIN_FP_TCH);

  if (fpOk && touch == HIGH) {
    if (!fingerHolding) {
      fingerHolding = true;
      Serial.println("\n[FP] Sentuhan terdeteksi (TCH=HIGH)! Menyalakan lampu & membaca sidik...");
      finger.LEDcontrol(true); // Nyalakan lampu biru sebelum memotret
      delay(60);
      showBigText("MEMBACA...");

      uint8_t p = FINGERPRINT_NOFINGER;
      unsigned long tStart = millis();
      while (p != FINGERPRINT_OK && (millis() - tStart < 1500)) {
        p = finger.getImage();
        delay(40);
      }

      if (p == FINGERPRINT_OK) {
        p = finger.image2Tz();
        if (p == FINGERPRINT_OK) {
          p = finger.fingerSearch();
          if (p == FINGERPRINT_OK) {
            lastScannedID = finger.fingerID;
            Serial.printf(" -> COCOK! Teridentifikasi ID #%d (Akurasi: %d)\n",
                          finger.fingerID, finger.confidence);
            showBigText("ID #" + String(finger.fingerID) + " OK");

            // Otomatis kirim data absensi ke server denanyar jika LAN aktif
            if (lanConnected) {
              syncWithServerDenanyar("absen", finger.fingerID);
            }
          } else {
            Serial.println(" -> Foto sidik sukses, tetapi BELUM TERDAFTAR di sensor.");
            showBigText("JARI OK!");
          }
        } else {
          Serial.println(" -> Gagal konversi citra sidik.");
          showBigText("ULANGI");
        }
      } else {
        Serial.println(" -> Gagal memotret sidik jari (timeout).");
        showBigText("TEMPEL RATA");
      }

      finger.LEDcontrol(false); // Matikan kembali lampu biru setelah selesai potret
    }
  } else if (touch == LOW && fingerHolding) {
    fingerHolding = false;
    Serial.println("[FP] Jari diangkat (TCH=LOW). Kembali ke standby.");
    finger.LEDcontrol(false);
    showBigText(adminMessage);
  }

  // 4. Heartbeat berkala & sinkronisasi perintah cloud server setiap 5 detik
  if (now - lastHeartbeat >= 5000) {
    lastHeartbeat = now;
    if (lanConnected) {
      Ethernet.maintain(); // Perpanjang sewa DHCP jika ada
      syncWithServerDenanyar("ping", 0); // Cek apakah admin SIAKAD mengirim instruksi baru
    }
    Serial.printf("[HEARTBEAT] Up: %lu s | FP: %s | TCH: %s | LAN: %s | IP: %s | Pesan: %s\n",
                  now / 1000,
                  fpOk ? "OK" : "ERR",
                  touch == HIGH ? "HIGH" : "LOW",
                  lanConnected ? "LINK ON" : "LINK OFF",
                  lanConnected ? Ethernet.localIP().toString().c_str() : "-",
                  adminMessage.c_str());
  }

  delay(15);
}
