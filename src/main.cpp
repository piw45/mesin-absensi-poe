#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Fingerprint.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Preferences.h>

// ==========================================
// PINOUT SIMETRIS 7 KIRI vs 7 KANAN
// ==========================================
// Sisi Kiri (Bawah ke Atas):
// 1. VIN  -> +5V
// 2. GND  -> Ground
// 3. D13  -> OLED SDA
// 4. D12  -> BUZZER (+)
// 5. D14  -> OLED SCL
// 6. D27  -> W5500 SCS / CS
// 7. D26  -> W5500 MOSI
// 8. D25  -> W5500 MISO
#define PIN_OLED_SDA   13
#define PIN_BUZZER     12
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
String lastDisplayedScan = "";
int lastScannedID = -1;
unsigned long statusHoldUntil = 0;
Preferences fpPrefs;
Preferences fpTplPrefs;
String lastEnrolledTemplateHex = "";

// ==========================================
// EKSTRAKSI & TRANSFER TEMPLATE 512-BYTE (SERVER <-> SENSOR RAM)
// ==========================================
String extractTemplateHexFromBuffer() {
  while (fpSerial.available()) fpSerial.read();
  if (finger.getModel() != FINGERPRINT_OK) return "";

  uint8_t tplBytes[512];
  int tplLen = 0;
  unsigned long t0 = millis();

  while (millis() - t0 < 2000 && tplLen < 512) {
    if (!fpSerial.available()) {
      delay(1);
      continue;
    }
    if (fpSerial.read() != 0xEF) continue;
    while (!fpSerial.available() && millis() - t0 < 2000) delay(1);
    if (fpSerial.read() != 0x01) continue;

    // Skip 4 byte address
    for (int a = 0; a < 4; a++) {
      while (!fpSerial.available() && millis() - t0 < 2000) delay(1);
      fpSerial.read();
    }
    while (!fpSerial.available() && millis() - t0 < 2000) delay(1);
    uint8_t pid = fpSerial.read();
    while (!fpSerial.available() && millis() - t0 < 2000) delay(1);
    uint8_t lenHi = fpSerial.read();
    while (!fpSerial.available() && millis() - t0 < 2000) delay(1);
    uint8_t lenLo = fpSerial.read();
    uint16_t pktLen = ((uint16_t)lenHi << 8) | lenLo;
    if (pktLen < 2) continue;
    uint16_t dataLen = pktLen - 2;

    for (uint16_t d = 0; d < dataLen; d++) {
      while (!fpSerial.available() && millis() - t0 < 2000) delay(1);
      uint8_t b = fpSerial.read();
      if ((pid == 0x02 || pid == 0x08) && tplLen < 512) {
        tplBytes[tplLen++] = b;
      }
    }
    // Baca 2 byte checksum
    for (int c = 0; c < 2; c++) {
      while (!fpSerial.available() && millis() - t0 < 2000) delay(1);
      fpSerial.read();
    }
    if (pid == 0x08) break;
  }

  if (tplLen < 512) return "";
  String hex = "";
  hex.reserve(1024);
  const char* hexChars = "0123456789ABCDEF";
  for (int i = 0; i < 512; i++) {
    hex += hexChars[(tplBytes[i] >> 4) & 0x0F];
    hex += hexChars[tplBytes[i] & 0x0F];
  }
  return hex;
}

bool loadTemplateHexToBuffer(const String& hexStr, uint8_t bufferSlot) {
  if (hexStr.length() < 1024) return false;
  uint8_t cmdData[2] = { 0x09, bufferSlot }; // 0x09 = PS_DownChar
  Adafruit_Fingerprint_Packet cmdPkt(FINGERPRINT_COMMANDPACKET, 2, cmdData);
  while (fpSerial.available()) fpSerial.read();
  finger.writeStructuredPacket(cmdPkt);
  if (finger.getStructuredPacket(&cmdPkt) != FINGERPRINT_OK) return false;
  if (cmdPkt.type != FINGERPRINT_ACKPACKET || cmdPkt.data[0] != FINGERPRINT_OK) return false;

  auto hexVal = [](char c) -> uint8_t {
    return (c >= '0' && c <= '9') ? (c - '0') :
           (c >= 'A' && c <= 'F') ? (c - 'A' + 10) :
           (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0;
  };

  // Kirim 4 paket data @ 128 byte = 512 byte
  for (int pkt = 0; pkt < 4; pkt++) {
    uint8_t pid = (pkt == 3) ? 0x08 : 0x02;
    uint16_t wireLen = 128 + 2;
    uint16_t sum = pid + (wireLen >> 8) + (wireLen & 0xFF);
    uint8_t hdr[9] = {
      0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
      pid, (uint8_t)(wireLen >> 8), (uint8_t)(wireLen & 0xFF)
    };
    fpSerial.write(hdr, 9);
    for (int i = 0; i < 128; i++) {
      int charIdx = (pkt * 128 + i) * 2;
      uint8_t b = (hexVal(hexStr[charIdx]) << 4) | hexVal(hexStr[charIdx + 1]);
      fpSerial.write(b);
      sum += b;
    }
    fpSerial.write((uint8_t)(sum >> 8));
    fpSerial.write((uint8_t)(sum & 0xFF));
    delay(8);
  }
  return true;
}

bool matchSensorBuffers(uint16_t& scoreOut) {
  uint8_t cmdData[1] = { 0x03 }; // 0x03 = PS_Match (bandingkan CharBuffer1 & CharBuffer2 di RAM)
  Adafruit_Fingerprint_Packet pkt(FINGERPRINT_COMMANDPACKET, 1, cmdData);
  finger.writeStructuredPacket(pkt);
  if (finger.getStructuredPacket(&pkt) != FINGERPRINT_OK) return false;
  if (pkt.type == FINGERPRINT_ACKPACKET && pkt.data[0] == FINGERPRINT_OK) {
    scoreOut = ((uint16_t)pkt.data[1] << 8) | pkt.data[2];
    return true;
  }
  return false;
}

String getSlotName(int slotId) {
  String key = "s" + String(slotId);
  String name = fpPrefs.getString(key.c_str(), "");
  if ((name.length() == 0 || name == "RIZAL") && slotId == 1) return "A. RIZAL KHABIBI YUNANTA";
  return name;
}

void saveSlotName(int slotId, String name) {
  name.trim();
  name.toUpperCase();
  if (slotId > 0 && name.length() > 0) {
    String key = "s" + String(slotId);
    if (fpPrefs.getString(key.c_str(), "") != name) {
      fpPrefs.putString(key.c_str(), name);
    }
  }
}

// Forward declaration
bool enrollFingerprint(int id);

unsigned long ledOffCooldownUntil = 0;

void setSensorStandby() {
  if (!fpOk) return;
  finger.LEDcontrol(false);
  ledOffCooldownUntil = millis() + 800; // Redam lonjakan sinyal TCH setelah lampu mati
}

void playPassiveTone(int freqHz, int durationMs) {
  if (freqHz <= 0 || durationMs <= 0) return;
  unsigned long periodUs = 1000000UL / freqHz;
  unsigned long highUs = periodUs / 12UL; // Duty cycle ~8% agar suara halus & tidak berisik
  if (highUs < 15) highUs = 15;
  unsigned long lowUs = periodUs - highUs;
  unsigned long totalCycles = ((unsigned long)durationMs * 1000UL) / periodUs;
  for (unsigned long c = 0; c < totalCycles; c++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delayMicroseconds(highUs);
    digitalWrite(PIN_BUZZER, LOW);
    delayMicroseconds(lowUs);
  }
  digitalWrite(PIN_BUZZER, LOW);
}

void beepBuzzer(int durationMs, int count = 1) {
  if (durationMs >= 150 && count == 1) {
    // Nada gagal / error (frekuensi rendah 850 Hz, singkat & halus)
    playPassiveTone(850, 140);
    return;
  }
  for (int i = 0; i < count; i++) {
    int freq = (count == 2 && i == 1) ? 2700 : 2200;
    playPassiveTone(freq, 45);
    if (i + 1 < count) delay(45);
  }
}

// ==========================================
// TAMPILAN OLED 1 BARIS HURUF BESAR
// ==========================================
void showBigText(String text) {
  if (!oledOk) return;
  text.trim();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2); // Selalu gunakan Font Size 2 (tinggi 16px, huruf besar 1 baris)

  int len = text.length();
  int textPixelW = len * 12 - 2;
  int y = (SCREEN_HEIGHT - 16) / 2;

  if (len <= 11) {
    display.clearDisplay();
    int x = (SCREEN_WIDTH - textPixelW) / 2;
    if (x < 0) x = 0;
    display.setCursor(x, y);
    display.print(text);
    display.display();
  } else {
    // Jika lebih dari 11 huruf (contoh: "SUKSES RIZAL"), tampilkan & geser halus 1 baris besar
    display.clearDisplay();
    display.setCursor(2, y);
    display.print(text);
    display.display();
    delay(350);

    int minX = SCREEN_WIDTH - textPixelW - 2;
    for (int x = 2; x >= minX; x -= 4) {
      display.clearDisplay();
      display.setCursor(x, y);
      display.print(text);
      display.display();
      delay(18);
    }
  }
}

// ==========================================
// KOMUNIKASI OTOMATIS KE SERVER DENANYAR
// ==========================================
bool uploadTemplateToServer(int slotId, int mhsId, const String& hexTpl) {
  if (!lanConnected || Ethernet.linkStatus() != LinkON || hexTpl.length() < 1024) return false;
  EthernetClient client;
  client.setTimeout(3000);
  if (!client.connect(serverHost, serverPort)) return false;

  String body = "template=" + hexTpl;
  String url = "/admin/api-mesin.php?action=upload_template&mac=DEADBEEFFEED&finger_id=" + String(slotId) + "&mahasiswa_id=" + String(mhsId);
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + serverHost + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + String(body.length()) + "\r\n" +
               "Connection: close\r\n\r\n" +
               body);

  unsigned long t0 = millis();
  while (client.connected() && millis() - t0 < 2500) {
    if (client.available()) client.read();
  }
  client.stop();
  Serial.printf("[SERVER] Template 512-byte ID #%d berhasil disimpan ke Database Server!\n", slotId);
  return true;
}

// Pencocokan langsung ke Database Server via RAM Sensor (CharBuffer1 vs CharBuffer2) TANPA menyimpan di sensor
int matchFingerAgainstServer(uint16_t& scoreOut) {
  if (!lanConnected || Ethernet.linkStatus() != LinkON) return -1;
  EthernetClient client;
  client.setTimeout(3000);
  if (!client.connect(serverHost, serverPort)) return -1;

  client.print(String("GET /admin/api-mesin.php?action=list_templates HTTP/1.1\r\n") +
               "Host: " + serverHost + "\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long t0 = millis();
  bool isBody = false;
  while ((client.connected() || client.available()) && millis() - t0 < 8000) {
    if (!client.available()) {
      delay(1);
      continue;
    }
    String line = client.readStringUntil('\n');
    line.trim();
    if (!isBody) {
      if (line.length() == 0) isBody = true;
      continue;
    }
    if (line == "END") break;
    if (line.startsWith("ITEM:")) {
      int c1 = 5;
      int c2 = line.indexOf(':', c1);
      if (c2 > c1) {
        int slotId = line.substring(c1, c2).toInt();
        String hexTpl = line.substring(c2 + 1);
        if (slotId > 0 && hexTpl.length() >= 1024) {
          uint16_t sc = 0;
          if (loadTemplateHexToBuffer(hexTpl, 2) && matchSensorBuffers(sc)) {
            scoreOut = sc;
            client.stop();
            return slotId;
          }
        }
      }
    }
  }
  client.stop();
  return -1;
}

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

    // 0. Sinkronkan daftar nama panggilan mahasantri per slot ("fp_map":"1:RIZAL,2:AHMAD")
    int mIdx = responseLine.indexOf("\"fp_map\":\"");
    if (mIdx >= 0) {
      int start = mIdx + 10;
      int end = responseLine.indexOf("\"", start);
      if (end > start) {
        String mapStr = responseLine.substring(start, end);
        int pos = 0;
        while (pos < (int)mapStr.length()) {
          int comma = mapStr.indexOf(',', pos);
          if (comma == -1) comma = mapStr.length();
          String pair = mapStr.substring(pos, comma);
          int colon = pair.indexOf(':');
          if (colon > 0) {
            int sId = pair.substring(0, colon).toInt();
            String sName = pair.substring(colon + 1);
            saveSlotName(sId, sName);
          }
          pos = comma + 1;
        }
      }
    }

    // Simpan panggilan jika ada di respon absen ("panggilan":"RIZAL")
    int pIdx = responseLine.indexOf("\"panggilan\":\"");
    if (pIdx >= 0 && fingerId > 0) {
      int start = pIdx + 13;
      int end = responseLine.indexOf("\"", start);
      if (end > start) {
        saveSlotName(fingerId, responseLine.substring(start, end));
      }
    }

    // 1. Perbarui teks layar jika server mengirimkan display baru (JSON format "display":"...")
    int dIdx = responseLine.indexOf("\"display\":\"");
    if (dIdx >= 0) {
      int start = dIdx + 11;
      int end = responseLine.indexOf("\"", start);
      if (end > start) {
        String newMsg = responseLine.substring(start, end);
        newMsg.trim();
        if (strcmp(action, "ping") == 0) {
          if (newMsg.length() > 0 && newMsg != adminMessage) {
            adminMessage = newMsg;
            if (millis() >= statusHoldUntil) {
              showBigText(adminMessage);
            }
            Serial.printf("[SERVER] Teks layar standby diperbarui: \"%s\"\n", adminMessage.c_str());
          }
        } else if (strcmp(action, "absen") == 0 && newMsg.length() > 0) {
          if (newMsg != lastDisplayedScan) {
            showBigText(newMsg);
            lastDisplayedScan = newMsg;
          }
          statusHoldUntil = millis() + 1800;
          Serial.printf("[SERVER] Tampilan presensi: \"%s\"\n", newMsg.c_str());
        }
      }
    }

    // 2. Jika server mengirimkan antrean perintah khusus (JSON format "cmd":"ENROLL:slot:mhs_id")
    int cIdx = responseLine.indexOf("\"cmd\":\"");
    if (cIdx >= 0) {
      int start = cIdx + 7;
      int end = responseLine.indexOf("\"", start);
      if (end > start) {
        String cmdData = responseLine.substring(start, end);
        cmdData.trim();

        if (cmdData.startsWith("ENROLL:")) {
          String enrollParams = cmdData.substring(7);
          int sep = enrollParams.indexOf(':');
          int enrollSlot = (sep > 0) ? enrollParams.substring(0, sep).toInt() : enrollParams.toInt();
          int mhsId = (sep > 0) ? enrollParams.substring(sep + 1).toInt() : 0;

          if (enrollSlot > 0) {
            Serial.printf("[SERVER CMD] Perintah pendaftaran jari dari Admin SIAKAD! ID #%d (Ref #%d)\n", enrollSlot, mhsId);
            bool ok = enrollFingerprint(enrollSlot);
            if (ok) {
              if (lastEnrolledTemplateHex.length() >= 1024) {
                uploadTemplateToServer(enrollSlot, mhsId, lastEnrolledTemplateHex);
              } else {
                String doneAction = "enroll_done&finger_id=" + String(enrollSlot) + "&mahasiswa_id=" + String(mhsId);
                syncWithServerDenanyar(doneAction.c_str(), 0);
              }
            }
          }
        } else if (cmdData.startsWith("DEL:")) {
          int delSlot = cmdData.substring(4).toInt();
          if (delSlot > 0) {
            Serial.printf("[SERVER CMD] Menghapus cache sidik jari ID #%d...\n", delSlot);
            if (delSlot <= 300) finger.deleteModel(delSlot);
            fpPrefs.remove(("s" + String(delSlot)).c_str());
            fpTplPrefs.remove(("t" + String(delSlot)).c_str());
            String doneAction = "del_done&finger_id=" + String(delSlot);
            syncWithServerDenanyar(doneAction.c_str(), 0);
          }
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
  if (!w5500Ok) {
    if (Ethernet.hardwareStatus() != EthernetNoHardware) {
      w5500Ok = true;
      Serial.println("\n[LAN] Modul W5500 terdeteksi aktif!");
    } else {
      return;
    }
  }

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
  if (!fpOk || id <= 0) return false;
  Serial.printf("\n[ENROLL] Memulai pendaftaran sidik jari untuk ID #%d...\n", id);
  finger.LEDcontrol(true);
  beepBuzzer(80, 1);
  showBigText("TEMPEL 1");

  // Langkah 1: Tempelkan jari pertama (timeout 12 detik)
  unsigned long start = millis();
  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (millis() - start > 12000) {
      setSensorStandby();
      beepBuzzer(200, 1);
      showBigText("TIMEOUT");
      delay(1500);
      showBigText(adminMessage);
      return false;
    }
    delay(40);
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    setSensorStandby();
    beepBuzzer(200, 1);
    showBigText("GAGAL 1");
    delay(1500);
    showBigText(adminMessage);
    return false;
  }

  // Langkah 2: Angkat jari
  beepBuzzer(45, 1);
  showBigText("ANGKAT");
  delay(1000);
  while (finger.getImage() != FINGERPRINT_NOFINGER) {
    delay(40);
  }

  // Langkah 3: Tempelkan jari yang sama sekali lagi untuk konfirmasi
  beepBuzzer(45, 1);
  showBigText("TEMPEL 2");
  start = millis();
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (millis() - start > 12000) {
      setSensorStandby();
      beepBuzzer(200, 1);
      showBigText("TIMEOUT");
      delay(1500);
      showBigText(adminMessage);
      return false;
    }
    delay(40);
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    setSensorStandby();
    beepBuzzer(200, 1);
    showBigText("GAGAL 2");
    delay(1500);
    showBigText(adminMessage);
    return false;
  }

  // Langkah 4: Cocokkan kedua foto sidik
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    setSensorStandby();
    beepBuzzer(200, 1);
    showBigText("BEDA JARI");
    delay(1500);
    showBigText(adminMessage);
    return false;
  }

  // Langkah 5: Ekstrak data jari dari RAM sensor untuk dikirim ke Server (TANPA simpan di sensor!)
  lastEnrolledTemplateHex = extractTemplateHexFromBuffer();
  setSensorStandby();
  if (lastEnrolledTemplateHex.length() >= 1024) {
    Serial.printf(" -> SUKSES! Pola jari ID #%d diekstrak dari RAM untuk dikirim ke Server (%d chars HEX).\n", id, lastEnrolledTemplateHex.length());
    beepBuzzer(45, 2);
    showBigText("ID #" + String(id) + " OK");
    delay(2000);
    showBigText(adminMessage);
    return true;
  } else {
    beepBuzzer(200, 1);
    showBigText("GAGAL BACA");
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
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  Serial.begin(115200);
  delay(1000);
  fpPrefs.begin("fp_names", false);
  fpTplPrefs.begin("fp_tpl", false);
  String curS1 = fpPrefs.getString("s1", "");
  if (curS1.length() == 0 || curS1 == "RIZAL") {
    fpPrefs.putString("s1", "A. RIZAL KHABIBI YUNANTA");
  }
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
    finger.emptyDatabase(); // Kosongkan memori internal sensor! Sensor murni hanya pembaca (0 penyimpanan lokal)
    fpTplPrefs.clear();
    Serial.println("    - Mode Murni Pembaca (Stateless): Memori Sensor Dikosongkan (0 data disimpan di sensor)");
    setSensorStandby(); // Langsung matikan lampu biru saat standby!
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

  // Inisialisasi hardware Ethernet controller
  Ethernet.begin(mac, staticIp);
  SPI.begin(PIN_W5500_SCK, PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CS);

  auto hw = Ethernet.hardwareStatus();
  if (hw != EthernetNoHardware) {
    w5500Ok = true;
    Serial.printf(" -> Modul W5500 Terdeteksi OK! (Tipe: %d)\n", (int)hw);
  } else {
    Serial.println(" -> Modul W5500 Tidak Terdeteksi.");
  }

  // Cek apakah LAN sudah dicolok dari awal
  checkLanConnection();

  // Pastikan sensor dalam mode standby (lampu mati)
  setSensorStandby();

  // Tampilkan layar standby 1 baris besar
  showBigText(adminMessage);
  beepBuzzer(45, 1);

  Serial.println("\n========================================");
  Serial.println("Setup Selesai! Layar siap dalam mode 1 baris.");
  Serial.println("========================================\n");
}

bool fingerHolding = false;
bool tchArmed = false;
unsigned long tchLowSince = 0;
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

  // 3. Sensor Sidik Jari (Mode Standby Lampu Mati + Anti-Pantulan TCH + Buzzer Hening)
  int touch = digitalRead(PIN_FP_TCH);

  // Arming: TCH harus stabil LOW minimal 300ms setelah masa cooldown lampu mati selesai
  if (touch == LOW) {
    if (tchLowSince == 0) tchLowSince = now;
    if (now >= ledOffCooldownUntil && (now - tchLowSince >= 300)) {
      tchArmed = true;
    }
    if (fingerHolding && now >= statusHoldUntil) {
      fingerHolding = false;
      Serial.println("[FP] Jari diangkat. Kembali ke teks standby.");
      showBigText(adminMessage);
    }
  } else {
    tchLowSince = 0;
  }

  if (fpOk && touch == HIGH && tchArmed && !fingerHolding && now >= ledOffCooldownUntil) {
    unsigned long tHighStart = millis();
    while (digitalRead(PIN_FP_TCH) == HIGH && millis() - tHighStart < 100) delay(1);
    unsigned long duration = millis() - tHighStart;

    if (duration >= 80) {
      tchArmed = false;
      fingerHolding = true;
      finger.LEDcontrol(true); // Nyalakan lampu biru saat disentuh
      delay(50);

      // Cek apakah benar-benar ada jari di kaca sensor TANPA bunyi dulu
      uint8_t p = FINGERPRINT_NOFINGER;
      unsigned long tStart = millis();
      while (p != FINGERPRINT_OK && (millis() - tStart < 900) && (digitalRead(PIN_FP_TCH) == HIGH)) {
        p = finger.getImage();
        if (p == FINGERPRINT_OK) break;
        delay(35);
      }

      if (p == FINGERPRINT_OK) {
        Serial.println("\n[FP] Sentuhan & jari nyata terdeteksi! Membaca sidik...");
        showBigText("MEMBACA...");
        p = finger.image2Tz(1);
        setSensorStandby(); // Matikan lampu biru segera setelah foto sidik diambil

        if (p == FINGERPRINT_OK) {
          uint16_t matchScore = 0;
          int matchedId = matchFingerAgainstServer(matchScore);
          if (matchedId > 0) {
            finger.fingerID = matchedId;
            finger.confidence = matchScore;
            lastScannedID = matchedId;
            String namaSantri = getSlotName(matchedId);
            String teksSukses = (namaSantri.length() > 0)
                                  ? ("SUKSES " + namaSantri)
                                  : ("SUKSES #" + String(matchedId));
            Serial.printf(" -> COCOK DARI SERVER! Teridentifikasi ID #%d (%s, Akurasi: %d)\n",
                          matchedId, teksSukses.c_str(), matchScore);
            lastDisplayedScan = teksSukses;
            beepBuzzer(45, 2);
            showBigText(teksSukses);
            statusHoldUntil = millis() + 1800;

            if (lanConnected) {
              syncWithServerDenanyar("absen", matchedId);
            }
          } else {
            Serial.println(" -> Foto sidik sukses, tetapi BELUM TERDAFTAR di server.");
            beepBuzzer(150, 1);
            showBigText("TDK TERDAFTAR");
            statusHoldUntil = millis() + 1500;
          }
        } else {
          Serial.println(" -> Gagal konversi citra sidik.");
          beepBuzzer(150, 1);
          showBigText("ULANGI");
          statusHoldUntil = millis() + 1200;
        }
      } else {
        // Hanya tersentuh sekilas / noise tanpa jari di kaca: matikan lampu kembali TANPA bunyi
        setSensorStandby();
        fingerHolding = false;
      }
    }
  }

  // 4. Heartbeat berkala & sinkronisasi perintah cloud server setiap 5 detik
  if (now - lastHeartbeat >= 5000) {
    lastHeartbeat = now;
    if (lanConnected) {
      Ethernet.maintain(); // Perpanjang sewa DHCP jika ada
      syncWithServerDenanyar("ping", 0); // Cek apakah admin SIAKAD mengirim instruksi baru
    }
    Serial.printf("[HEARTBEAT] Up: %lu s | FP: %s | TCH: %s | W5500: %s | LAN: %s | IP: %s | Pesan: %s\n",
                  now / 1000,
                  fpOk ? "OK" : "ERR",
                  digitalRead(PIN_FP_TCH) == HIGH ? "HIGH" : "LOW",
                  w5500Ok ? "OK" : "ERR",
                  lanConnected ? "LINK ON" : "LINK OFF",
                  lanConnected ? Ethernet.localIP().toString().c_str() : "-",
                  adminMessage.c_str());
  }

  delay(15);
}
