#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Fingerprint.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Dns.h>
#include <Preferences.h>
#include <mbedtls/md.h>
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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
// SERTIFIKAT ROOT CA (ISRG Root X1 - Let's Encrypt)
// ==========================================
static const char ISRG_ROOT_X1_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// ==========================================
// OLED 0.91 INCH (128 x 32 PIXEL)
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, 100000, 100000);

HardwareSerial fpSerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fpSerial);

// ==========================================
// KONFIGURASI JARINGAN, IDENTITAS & SERVER
// ==========================================
enum NetMode { NET_NONE, NET_LAN, NET_WIFI };

byte mac[6] = { 0 };
IPAddress staticIp(192, 168, 1, 199);
IPAddress cachedServerIp(0, 0, 0, 0);
unsigned long lastDnsResolve = 0;

String deviceId = "";
String shortId  = "";
String apSsid   = "";

String savedSsid  = "";
String savedPass  = "";
String apiSecret  = "";
String serverHost = "denanyar.ac.id";
String buzzMode   = "auto";
const int lanHttpPort = 80;
const int wifiHttpsPort = 443;

volatile NetMode activeNet = NET_NONE;
bool oledOk = false;
uint8_t activeOledAddr = 0x3C;
bool fpOk = false;
bool w5500Ok = false;
bool lanConnected = false;
bool wifiConnecting = false;
unsigned long lastWifiAttempt = 0;
unsigned long wifiConnectStart = 0;
uint8_t wifiFailCount = 0;
uint8_t lastWifiReason = 0;
String lastWifiError = "";
bool initialPingSent = false;

struct CachedTemplate {
  uint16_t slotId;
  uint8_t data[512];
};
static const size_t MAX_CACHED_TEMPLATES = 120;

SemaphoreHandle_t netMutex = nullptr;
SemaphoreHandle_t stateMutex = nullptr;
std::vector<CachedTemplate> tplCache;
bool tplCacheSynced = false;
bool tplCacheComplete = false;
String cachedTplVer = "";
volatile bool bgPaused = false;
volatile bool bgForcePing = false;
volatile bool bgForceCacheSync = false;

bool hasPendingPing = false;
String pendingDisplay = "";
String pendingCmd = "";
String pendingFpMap = "";
bool pendingAuthFail = false;

bool setupPortalActive = false;
bool setupPortalManual = false;
unsigned long setupPortalStart = 0;
unsigned long portalIdleSince = 0;
unsigned long setupOledCycleAt = 0;
uint8_t setupOledPage = 0;
DNSServer dnsServer;
WebServer setupWebServer(80);

// Teks tampilan utama yang dapat dikontrol Admin (1 baris besar)
String adminMessage = "TEMPEL JARI";
String lastDisplayedScan = "";
int lastScannedID = -1;
unsigned long statusHoldUntil = 0;
bool oledNeedsReset = false;

Preferences fpPrefs;
Preferences fpTplPrefs;
Preferences netPrefs;
String lastEnrolledTemplateHex = "";
String serialLineBuf = "";

// Forward declarations
bool enrollFingerprint(int id);
bool syncWithServerDenanyar(const char* action, int fingerId, const char* extraParams = nullptr);
void startSetupPortal(bool manualTrigger);
void stopSetupPortal();

// ==========================================
// UTILITAS KRIPTOGRAFI & WAKTU
// ==========================================
String sha256Hex(const String& data) {
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  String out;
  out.reserve(64);
  const char* hex = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    out += hex[(hash[i] >> 4) & 0x0F];
    out += hex[hash[i] & 0x0F];
  }
  return out;
}

String hmacSha256Hex(const String& key, const String& msg) {
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, reinterpret_cast<const unsigned char*>(key.c_str()), key.length());
  mbedtls_md_hmac_update(&ctx, reinterpret_cast<const unsigned char*>(msg.c_str()), msg.length());
  mbedtls_md_hmac_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  String out;
  out.reserve(64);
  const char* hex = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    out += hex[(hash[i] >> 4) & 0x0F];
    out += hex[hash[i] & 0x0F];
  }
  return out;
}

bool setRtcEpoch(unsigned long epoch) {
  if (epoch < 1700000000UL) return false;
  struct timeval tv;
  tv.tv_sec = (time_t)epoch;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  return true;
}

bool isClockValid() {
  return (unsigned long)time(nullptr) >= 1700000000UL;
}

unsigned long extractJsonUlong(const String& json, const char* key) {
  String pat = String("\"") + key + "\":";
  int idx = json.indexOf(pat);
  if (idx < 0) return 0;
  int pos = idx + pat.length();
  while (pos < (int)json.length() && (json[pos] == ' ' || json[pos] == '"')) pos++;
  unsigned long val = 0;
  bool anyDigit = false;
  while (pos < (int)json.length() && isDigit((unsigned char)json[pos])) {
    anyDigit = true;
    val = val * 10UL + (unsigned long)(json[pos] - '0');
    pos++;
  }
  return anyDigit ? val : 0;
}

String extractJsonString(const String& json, const char* key) {
  String pat = String("\"") + key + "\":\"";
  int idx = json.indexOf(pat);
  if (idx < 0) return "";
  int start = idx + pat.length();
  int end = json.indexOf("\"", start);
  if (end <= start) return "";
  return json.substring(start, end);
}

void recoverI2cBus(int* outSdaLow = nullptr, int* outSclLow = nullptr) {
  pinMode(PIN_OLED_SDA, INPUT_PULLUP);
  pinMode(PIN_OLED_SCL, INPUT | OUTPUT_OPEN_DRAIN | PULLUP);
  for (int i = 0; i < 9; i++) {
    digitalWrite(PIN_OLED_SCL, LOW);
    delayMicroseconds(10);
    if (i == 0 && outSclLow) *outSclLow = digitalRead(PIN_OLED_SCL);
    digitalWrite(PIN_OLED_SCL, HIGH);
    delayMicroseconds(10);
  }
  pinMode(PIN_OLED_SDA, INPUT | OUTPUT_OPEN_DRAIN | PULLUP);
  digitalWrite(PIN_OLED_SDA, LOW);
  delayMicroseconds(10);
  if (outSdaLow) *outSdaLow = digitalRead(PIN_OLED_SDA);
  digitalWrite(PIN_OLED_SCL, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_OLED_SDA, HIGH);
  delayMicroseconds(10);
  pinMode(PIN_OLED_SDA, INPUT_PULLUP);
  pinMode(PIN_OLED_SCL, INPUT_PULLUP);
}

String currentIpString() {
  if (activeNet == NET_LAN) return Ethernet.localIP().toString();
  if (activeNet == NET_WIFI) return WiFi.localIP().toString();
  return "-";
}

const char* currentNetName() {
  if (activeNet == NET_LAN) return "LAN";
  if (activeNet == NET_WIFI) return "WIFI";
  return "NONE";
}

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

bool hexToBytes512(const String& hexStr, uint8_t out[512]) {
  if (hexStr.length() < 1024) return false;
  auto hexVal = [](char c) -> uint8_t {
    return (c >= '0' && c <= '9') ? (c - '0') :
           (c >= 'A' && c <= 'F') ? (c - 'A' + 10) :
           (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0;
  };
  for (int i = 0; i < 512; i++) {
    int idx = i * 2;
    out[i] = (hexVal(hexStr[idx]) << 4) | hexVal(hexStr[idx + 1]);
  }
  return true;
}

bool loadTemplateBytesToBuffer(const uint8_t tpl[512], uint8_t bufferSlot) {
  uint8_t cmdData[2] = { 0x09, bufferSlot }; // 0x09 = PS_DownChar
  Adafruit_Fingerprint_Packet cmdPkt(FINGERPRINT_COMMANDPACKET, 2, cmdData);
  while (fpSerial.available()) fpSerial.read();
  finger.writeStructuredPacket(cmdPkt);
  if (finger.getStructuredPacket(&cmdPkt) != FINGERPRINT_OK) return false;
  if (cmdPkt.type != FINGERPRINT_ACKPACKET || cmdPkt.data[0] != FINGERPRINT_OK) return false;

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
      uint8_t b = tpl[pkt * 128 + i];
      fpSerial.write(b);
      sum += b;
    }
    fpSerial.write((uint8_t)(sum >> 8));
    fpSerial.write((uint8_t)(sum & 0xFF));
    delay(8);
  }
  return true;
}

bool loadTemplateHexToBuffer(const String& hexStr, uint8_t bufferSlot) {
  uint8_t tpl[512];
  if (!hexToBytes512(hexStr, tpl)) return false;
  return loadTemplateBytesToBuffer(tpl, bufferSlot);
}

bool matchSensorBuffers(uint16_t& scoreOut) {
  uint8_t cmdData[1] = { 0x03 }; // 0x03 = PS_Match (CharBuffer1 vs CharBuffer2)
  Adafruit_Fingerprint_Packet pkt(FINGERPRINT_COMMANDPACKET, 1, cmdData);
  finger.writeStructuredPacket(pkt);
  if (finger.getStructuredPacket(&pkt) != FINGERPRINT_OK) return false;
  if (pkt.type == FINGERPRINT_ACKPACKET && pkt.data[0] == FINGERPRINT_OK) {
    scoreOut = ((uint16_t)pkt.data[1] << 8) | pkt.data[2];
    return true;
  }
  return false;
}

int matchFingerFromRamCache(uint16_t& scoreOut) {
  if (!stateMutex) return -1;
  uint8_t localTpl[512];
  for (size_t i = 0;; i++) {
    uint16_t slotId = 0;
    bool hasEntry = false;
    if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      if (i < tplCache.size()) {
        slotId = tplCache[i].slotId;
        memcpy(localTpl, tplCache[i].data, 512);
        hasEntry = true;
      }
      xSemaphoreGive(stateMutex);
    }
    if (!hasEntry) break;

    uint16_t sc = 0;
    if (loadTemplateBytesToBuffer(localTpl, 2) && matchSensorBuffers(sc)) {
      scoreOut = sc;
      return (int)slotId;
    }
  }
  return -1;
}

String getSlotName(int slotId) {
  String key = "s" + String(slotId);
  return fpPrefs.getString(key.c_str(), "");
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

unsigned long ledOffCooldownUntil = 0;

void setSensorStandby() {
  if (!fpOk) return;
  finger.LEDcontrol(false);
  ledOffCooldownUntil = millis() + 300;
}

void playPassiveTone(int freqHz, int durationMs) {
  if (freqHz <= 0 || durationMs <= 0) return;
  unsigned long periodUs = 1000000UL / freqHz;
  unsigned long highUs = periodUs / 12UL;
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
    if (buzzMode != "passive") {
      digitalWrite(PIN_BUZZER, HIGH);
      delay(220);
      digitalWrite(PIN_BUZZER, LOW);
    }
    if (buzzMode != "active") {
      playPassiveTone(850, 140);
    }
    return;
  }
  for (int i = 0; i < count; i++) {
    int activeMs = (durationMs >= 60) ? durationMs : 60;
    if (buzzMode != "passive") {
      digitalWrite(PIN_BUZZER, HIGH);
      delay(activeMs);
      digitalWrite(PIN_BUZZER, LOW);
    }
    if (buzzMode != "active") {
      int freq = (count == 2 && i == 1) ? 2700 : 2200;
      playPassiveTone(freq, 45);
    }
    if (i + 1 < count) delay(60);
  }
}

// ==========================================
// INISIALISASI, PEMULIHAN & TAMPILAN OLED 1 BARIS HURUF BESAR
// ==========================================
unsigned long lastOledRecoverMs = 0;

bool initOledDisplay(bool isBoot = false) {
  // Jangan batasi recovery pertama setelah boot bila gagal belakangan.
  lastOledRecoverMs = isBoot ? 0 : millis();
  // Wire.end() wajib agar TwoWire::begin() mengikat ulang pin setelah recoverI2cBus().
  Wire.end();
  delay(5);
  pinMode(PIN_OLED_SDA, INPUT_PULLUP);
  pinMode(PIN_OLED_SCL, INPUT_PULLUP);
  delayMicroseconds(20);
  int sdaPre = digitalRead(PIN_OLED_SDA);
  int sclPre = digitalRead(PIN_OLED_SCL);
  int sdaLow = -1, sclLow = -1;
  recoverI2cBus(&sdaLow, &sclLow);
  int sdaPost = digitalRead(PIN_OLED_SDA);
  int sclPost = digitalRead(PIN_OLED_SCL);

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL, 100000);
  Wire.setTimeOut(50);
  delay(isBoot ? 180 : 35);

  uint8_t foundAddr = 0;
  uint8_t err3C = 99, err3D = 99;
  int maxAttempts = isBoot ? 5 : 2;
  for (int attempt = 0; attempt < maxAttempts && foundAddr == 0; attempt++) {
    for (uint8_t candAddr : { (uint8_t)0x3C, (uint8_t)0x3D }) {
      Wire.beginTransmission(candAddr);
      uint8_t e = Wire.endTransmission();
      if (candAddr == 0x3C) err3C = e;
      else err3D = e;
      if (e == 0) {
        foundAddr = candAddr;
        break;
      }
    }
    if (foundAddr == 0 && attempt + 1 < maxAttempts) {
      delay(isBoot ? 80 : 20);
    }
  }

  if (foundAddr == 0) {
    oledOk = false;
    if (!isBoot) {
      Serial.printf("[OLED] Recovery gagal (pre:%d/%d low:%d/%d post:%d/%d err3C=%u err3D=%u)\n",
                    sdaPre, sclPre, sdaLow, sclLow, sdaPost, sclPost, err3C, err3D);
    }
    return false;
  }

  activeOledAddr = foundAddr;
  display.begin(SSD1306_SWITCHCAPVCC, activeOledAddr, true, false);
  display.dim(false);
  display.ssd1306_command(SSD1306_CHARGEPUMP);
  display.ssd1306_command(0x14);
  display.ssd1306_command(SSD1306_DISPLAYON);
  oledOk = true;
  return true;
}

void renderOledFrame(const String& text, int x, int y) {
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.clearDisplay();
  display.setCursor(x, y);
  display.print(text);
  display.display();
}

void showBigText(String text) {
  if (!oledOk) return;
  text.trim();

  Wire.beginTransmission(activeOledAddr);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    delay(5);
    Wire.beginTransmission(activeOledAddr);
    err = Wire.endTransmission();
  }
  if (err != 0) {
    Serial.printf("[OLED] Gangguan I2C (err=%u) sebelum update \"%s\", memulihkan...\n", err, text.c_str());
    if (lastOledRecoverMs == 0 || millis() - lastOledRecoverMs >= 1500UL) {
      initOledDisplay(false);
    } else {
      oledOk = false;
    }
    if (!oledOk) return;
  }

  int len = text.length();
  int textPixelW = len * 12 - 2;
  int y = (SCREEN_HEIGHT - 16) / 2;

  if (len <= 11) {
    int x = (SCREEN_WIDTH - textPixelW) / 2;
    if (x < 0) x = 0;
    renderOledFrame(text, x, y);

    Wire.beginTransmission(activeOledAddr);
    if (Wire.endTransmission() != 0 && (lastOledRecoverMs == 0 || millis() - lastOledRecoverMs >= 1500UL)) {
      if (initOledDisplay(false) && oledOk) {
        renderOledFrame(text, x, y);
      }
    }
  } else {
    renderOledFrame(text, 2, y);
    delay(350);

    int minX = SCREEN_WIDTH - textPixelW - 2;
    for (int x = 2; x >= minX; x -= 4) {
      renderOledFrame(text, x, y);
      delay(18);
    }
  }
}

// ==========================================
// KONEKSI SERVER & SINKRONISASI JAM (NTP + HTTP FALLBACK)
// ==========================================
bool connectLanToServer(EthernetClient& client, uint16_t timeoutMs = 800) {
  client.setTimeout(timeoutMs);
  if ((uint32_t)cachedServerIp == 0 || millis() - lastDnsResolve > 300000UL) {
    DNSClient dns;
    dns.begin(Ethernet.dnsServerIP());
    if (dns.getHostByName(serverHost.c_str(), cachedServerIp, 1500) == 1) {
      lastDnsResolve = millis();
    }
  }
  if ((uint32_t)cachedServerIp != 0 && client.connect(cachedServerIp, lanHttpPort)) {
    return true;
  }
  if (client.connect(serverHost.c_str(), lanHttpPort)) {
    return true;
  }
  cachedServerIp = IPAddress(0, 0, 0, 0);
  return false;
}

bool readLineWithDeadline(Client& client, String& lineOut, unsigned long deadlineMs) {
  lineOut = "";
  bool gotAny = false;
  while ((long)(millis() - deadlineMs) < 0) {
    int avail = client.available();
    if (avail > 0) {
      int c = client.read();
      if (c < 0) continue;
      gotAny = true;
      if (c == '\n') return true;
      if (c != '\r' && lineOut.length() < 4096) {
        lineOut += (char)c;
      }
    } else {
      if (!client.connected()) {
        return gotAny;
      }
      delay(2);
    }
  }
  return gotAny;
}

bool fetchHttpServerTime(Client& client) {
  client.print(String("GET /admin/api-mesin.php?action=time HTTP/1.0\r\n") +
               "Host: " + serverHost + "\r\n" +
               "User-Agent: MesinAbsensi/2.0\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long deadline = millis() + 3000UL;
  bool isBody = false;
  String body = "";
  String line = "";
  while ((long)(millis() - deadline) < 0 && readLineWithDeadline(client, line, deadline)) {
    line.trim();
    if (!isBody) {
      if (line.length() == 0) isBody = true;
      continue;
    }
    body += line;
  }
  client.stop();

  unsigned long epoch = extractJsonUlong(body, "time");
  if (setRtcEpoch(epoch)) {
    Serial.printf("[CLOCK] Jam disinkronkan via HTTP server: %lu\n", epoch);
    return true;
  }
  return false;
}

bool syncLanNtp() {
  IPAddress ntpIp(0, 0, 0, 0);
  DNSClient dns;
  dns.begin(Ethernet.dnsServerIP());
  if (dns.getHostByName("pool.ntp.org", ntpIp, 1500) != 1) {
    return false;
  }

  EthernetUDP udp;
  if (!udp.begin(8888)) return false;

  uint8_t pkt[48];
  memset(pkt, 0, sizeof(pkt));
  pkt[0] = 0b11100011; // LI, Version, Mode
  pkt[2] = 6;
  pkt[3] = 0xEC;

  udp.beginPacket(ntpIp, 123);
  udp.write(pkt, 48);
  udp.endPacket();

  unsigned long t0 = millis();
  while (millis() - t0 < 2500) {
    int sz = udp.parsePacket();
    if (sz >= 48) {
      udp.read(pkt, 48);
      udp.stop();
      unsigned long hi = word(pkt[40], pkt[41]);
      unsigned long lo = word(pkt[42], pkt[43]);
      unsigned long secsSince1900 = (hi << 16) | lo;
      const unsigned long seventyYears = 2208988800UL;
      if (secsSince1900 > seventyYears) {
        unsigned long epoch = secsSince1900 - seventyYears;
        if (setRtcEpoch(epoch)) {
          Serial.printf("[CLOCK] Jam LAN disinkronkan via NTP: %lu\n", epoch);
          return true;
        }
      }
      return false;
    }
    delay(10);
  }
  udp.stop();
  return false;
}

bool syncClockIfNeeded() {
  if (isClockValid()) return true;
  if (activeNet == NET_NONE) return false;

  if (activeNet == NET_WIFI) {
    Serial.println("[CLOCK] Sinkronisasi jam NTP via Wi-Fi...");
    configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
    unsigned long t0 = millis();
    while (millis() - t0 < 3500) {
      if (isClockValid()) {
        Serial.printf("[CLOCK] Jam Wi-Fi sinkron via NTP: %lu\n", (unsigned long)time(nullptr));
        return true;
      }
      delay(100);
    }
    WiFiClient plainClient;
    plainClient.setTimeout(3);
    if (plainClient.connect(serverHost.c_str(), lanHttpPort, 3000)) {
      return fetchHttpServerTime(plainClient);
    }
    return false;
  }

  if (activeNet == NET_LAN) {
    Serial.println("[CLOCK] Sinkronisasi jam NTP via LAN...");
    if (syncLanNtp()) return true;
    EthernetClient client;
    if (connectLanToServer(client, 1500)) {
      return fetchHttpServerTime(client);
    }
    return false;
  }

  return false;
}

// ==========================================
// SIGNED HTTP/HTTPS TRANSPORT (HMAC-SHA256)
// ==========================================
void writeSignedPostHeadersAndBody(Client& client, const String& body) {
  String tsStr = String((unsigned long)time(nullptr));
  String bodyHash = sha256Hex(body);
  String canonical = "POST\n/admin/api-mesin.php\n" + tsStr + "\n" + bodyHash;
  String sig = hmacSha256Hex(apiSecret, canonical);

  String hdr;
  hdr.reserve(320);
  hdr  = "POST /admin/api-mesin.php HTTP/1.0\r\n";
  hdr += "Host: " + serverHost + "\r\n";
  hdr += "User-Agent: MesinAbsensi/2.0\r\n";
  hdr += "Content-Type: application/x-www-form-urlencoded\r\n";
  hdr += "Content-Length: " + String(body.length()) + "\r\n";
  hdr += "X-Device-Id: " + deviceId + "\r\n";
  hdr += "X-Timestamp: " + tsStr + "\r\n";
  hdr += "X-Signature: " + sig + "\r\n";
  hdr += "Connection: close\r\n\r\n";

  client.print(hdr);
  if (body.length() > 0) {
    client.print(body);
  }
}

bool readHttpStatusAndBody(Client& client, int& httpCodeOut, String& bodyOut, unsigned long timeoutMs) {
  httpCodeOut = 0;
  bodyOut = "";
  unsigned long deadline = millis() + timeoutMs;
  bool gotStatus = false;
  bool isBody = false;
  String line = "";

  while (!isBody && (long)(millis() - deadline) < 0) {
    if (!readLineWithDeadline(client, line, deadline)) {
      break;
    }
    line.trim();
    if (!gotStatus && line.startsWith("HTTP/")) {
      int sp = line.indexOf(' ');
      if (sp > 0) {
        httpCodeOut = line.substring(sp + 1).toInt();
      }
      gotStatus = true;
    } else if (line.length() == 0) {
      isBody = true;
    }
  }

  uint8_t buf[256];
  while (isBody && (long)(millis() - deadline) < 0) {
    int avail = client.available();
    if (avail > 0) {
      int toRead = avail > (int)sizeof(buf) ? (int)sizeof(buf) : avail;
      int n = client.read(buf, toRead);
      if (n > 0) {
        for (int i = 0; i < n; i++) {
          bodyOut += (char)buf[i];
        }
      }
    } else {
      if (!client.connected()) {
        break;
      }
      delay(2);
    }
  }
  bodyOut.trim();
  client.stop();
  return gotStatus;
}

bool postSignedApi(const String& body, String& responseOut, int& httpCodeOut, unsigned long timeoutMs = 4500) {
  responseOut = "";
  httpCodeOut = 0;

  if (activeNet == NET_NONE) return false;
  if (apiSecret.length() == 0) {
    Serial.println("[AUTH] API Secret belum diisi! Gunakan Serial 'SECRET <token>' atau Captive Portal.");
    return false;
  }
  if (netMutex && xSemaphoreTake(netMutex, pdMS_TO_TICKS(9000)) != pdTRUE) {
    return false;
  }

  bool ok = false;
  if (!syncClockIfNeeded()) {
    Serial.println("[CLOCK] Gagal sinkronisasi jam sebelum request bertanda tangan.");
    if (netMutex) xSemaphoreGive(netMutex);
    return false;
  }

  for (int attempt = 0; attempt < 2; attempt++) {
    bool sent = false;

    if (activeNet == NET_WIFI) {
      WiFiClientSecure client;
      client.setCACert(ISRG_ROOT_X1_CA);
      client.setTimeout(5);
      client.setHandshakeTimeout(6);
      if (client.connect(serverHost.c_str(), wifiHttpsPort, 5000)) {
        writeSignedPostHeadersAndBody(client, body);
        sent = readHttpStatusAndBody(client, httpCodeOut, responseOut, timeoutMs > 6000 ? timeoutMs : 6000);
      } else {
        Serial.println("[HTTPS] Koneksi TLS ke server gagal.");
        break;
      }
    } else if (activeNet == NET_LAN) {
      EthernetClient client;
      if (connectLanToServer(client, 1000)) {
        writeSignedPostHeadersAndBody(client, body);
        sent = readHttpStatusAndBody(client, httpCodeOut, responseOut, timeoutMs);
      } else {
        break;
      }
    }

    if (!sent) break;

    if (httpCodeOut == 401 && attempt == 0) {
      unsigned long srvEpoch = extractJsonUlong(responseOut, "server_time");
      if (setRtcEpoch(srvEpoch)) {
        Serial.printf("[CLOCK] Jam dikoreksi dari respon 401 server (%lu), mencoba ulang...\n", srvEpoch);
        continue;
      }
    }
    ok = true;
    break;
  }

  if (netMutex) xSemaphoreGive(netMutex);
  return ok;
}

bool uploadTemplateToServer(int slotId, int mhsId, const String& hexTpl) {
  if (activeNet == NET_NONE || hexTpl.length() < 1024) return false;

  String body = "action=upload_template&finger_id=" + String(slotId) +
                "&mahasiswa_id=" + String(mhsId) +
                "&template=" + hexTpl;
  String resp;
  int httpCode = 0;
  if (!postSignedApi(body, resp, httpCode, 7000)) return false;

  Serial.printf("[SERVER] Upload template slot #%d -> HTTP %d: %s\n", slotId, httpCode, resp.c_str());
  return httpCode == 200;
}

// Pencocokan langsung ke Database Server via RAM Sensor (CharBuffer1 vs CharBuffer2) TANPA menyimpan di sensor
int matchFingerAgainstServer(uint16_t& scoreOut) {
  if (activeNet == NET_NONE || apiSecret.length() == 0) return -1;
  if (netMutex && xSemaphoreTake(netMutex, pdMS_TO_TICKS(10000)) != pdTRUE) return -1;

  int matchedResult = -1;
  if (syncClockIfNeeded()) {
    String body = "action=list_templates";

    auto streamTemplates = [&](Client& client, unsigned long streamTimeoutMs) -> int {
      writeSignedPostHeadersAndBody(client, body);

      unsigned long deadline = millis() + streamTimeoutMs;
      bool gotStatus = false;
      int httpCode = 0;
      bool isBody = false;
      String line = "";

      while ((long)(millis() - deadline) < 0 && readLineWithDeadline(client, line, deadline)) {
        line.trim();
        if (!isBody) {
          if (!gotStatus && line.startsWith("HTTP/")) {
            int sp = line.indexOf(' ');
            if (sp > 0) httpCode = line.substring(sp + 1).toInt();
            gotStatus = true;
          } else if (line.length() == 0) {
            isBody = true;
            if (httpCode != 200) {
              Serial.printf("[AUTH/MATCH] list_templates HTTP %d\n", httpCode);
              client.stop();
              return -1;
            }
          }
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
    };

    if (activeNet == NET_WIFI) {
      WiFiClientSecure client;
      client.setCACert(ISRG_ROOT_X1_CA);
      client.setTimeout(6);
      client.setHandshakeTimeout(6);
      if (client.connect(serverHost.c_str(), wifiHttpsPort, 5000)) {
        matchedResult = streamTemplates(client, 12000);
      }
    } else if (activeNet == NET_LAN) {
      EthernetClient client;
      if (connectLanToServer(client, 1000)) {
        matchedResult = streamTemplates(client, 8000);
      }
    }
  }

  if (netMutex) xSemaphoreGive(netMutex);
  return matchedResult;
}

void applyFpMapCsv(const String& mapStr) {
  if (mapStr.length() == 0) return;
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

void processServerCommand(const String& cmdData) {
  if (cmdData.length() == 0) return;

  if (cmdData.startsWith("ENROLL:")) {
    String enrollParams = cmdData.substring(7);
    int sep = enrollParams.indexOf(':');
    int enrollSlot = (sep > 0) ? enrollParams.substring(0, sep).toInt() : enrollParams.toInt();
    int mhsId = (sep > 0) ? enrollParams.substring(sep + 1).toInt() : 0;

    if (enrollSlot > 0) {
      bgPaused = true;
      Serial.printf("[SERVER CMD] Pendaftaran jari dari Admin SIAKAD! Slot #%d (Ref #%d)\n", enrollSlot, mhsId);
      bool ok = enrollFingerprint(enrollSlot);
      if (ok) {
        if (lastEnrolledTemplateHex.length() >= 1024) {
          uploadTemplateToServer(enrollSlot, mhsId, lastEnrolledTemplateHex);
        } else {
          String extra = "mahasiswa_id=" + String(mhsId);
          syncWithServerDenanyar("enroll_done", enrollSlot, extra.c_str());
        }
        bgForceCacheSync = true;
      } else {
        syncWithServerDenanyar("enroll_cancel", 0);
      }
      bgForcePing = true;
      bgPaused = false;
    }
  } else if (cmdData.startsWith("DEL:")) {
    int delSlot = cmdData.substring(4).toInt();
    if (delSlot > 0) {
      bgPaused = true;
      Serial.printf("[SERVER CMD] Menghapus cache sidik jari Slot #%d...\n", delSlot);
      if (delSlot <= 300) finger.deleteModel(delSlot);
      fpPrefs.remove(("s" + String(delSlot)).c_str());
      fpTplPrefs.remove(("t" + String(delSlot)).c_str());
      syncWithServerDenanyar("del_done", delSlot);
      bgForceCacheSync = true;
      bgForcePing = true;
      bgPaused = false;
    }
  }
}

bool syncWithServerDenanyar(const char* action, int fingerId, const char* extraParams) {
  if (activeNet == NET_NONE) return false;

  if (apiSecret.length() == 0) {
    Serial.println("[AUTH] API Secret kosong. Hubungkan Serial Monitor atau buka Captive Portal untuk isi secret.");
    if (strcmp(action, "ping") == 0 && millis() >= statusHoldUntil) {
      showBigText("SET SECRET");
    }
    return false;
  }

  String body = "action=" + String(action) +
                "&ip=" + currentIpString() +
                "&koneksi=" + String(currentNetName());
  if (fingerId > 0) {
    body += "&finger_id=" + String(fingerId);
  }
  if (extraParams && strlen(extraParams) > 0) {
    body += "&" + String(extraParams);
  }

  Serial.printf("\n[SERVER:%s] POST %s (Aksi: %s)...\n", currentNetName(), serverHost.c_str(), action);

  String responseLine;
  int httpCode = 0;
  if (!postSignedApi(body, responseLine, httpCode, 4500)) {
    Serial.println(" -> Server denanyar belum merespon atau offline.");
    return false;
  }

  Serial.printf(" -> HTTP %d: %s\n", httpCode, responseLine.c_str());

  if (httpCode == 401 || httpCode == 403 || httpCode == 429) {
    if (httpCode == 401 && millis() >= statusHoldUntil) {
      showBigText("AUTH GAGAL");
      statusHoldUntil = millis() + 2000;
      oledNeedsReset = true;
    }
    return false;
  }

  if (responseLine.length() == 0) return false;

  applyFpMapCsv(extractJsonString(responseLine, "fp_map"));

  String panggilan = extractJsonString(responseLine, "panggilan");
  if (panggilan.length() > 0 && fingerId > 0) {
    saveSlotName(fingerId, panggilan);
  }

  bool isErr = (responseLine.indexOf("\"status\":\"error\"") >= 0);
  String newMsg = extractJsonString(responseLine, "display");
  newMsg.trim();
  if (newMsg.length() > 0) {
    if (strcmp(action, "ping") == 0) {
      if (newMsg != adminMessage) {
        adminMessage = newMsg;
        if (millis() >= statusHoldUntil) {
          showBigText(adminMessage);
        }
        Serial.printf("[SERVER] Teks layar standby diperbarui: \"%s\"\n", adminMessage.c_str());
      }
    } else if (strcmp(action, "absen") == 0) {
      if (isErr) {
        beepBuzzer(150, 1);
      } else {
        beepBuzzer(45, 2);
      }
      showBigText(newMsg);
      lastDisplayedScan = newMsg;
      statusHoldUntil = millis() + 2000;
      Serial.printf("[SERVER] Tampilan presensi: \"%s\"\n", newMsg.c_str());
    }
  }

  String cmdData = extractJsonString(responseLine, "cmd");
  cmdData.trim();
  if (cmdData.length() > 0) {
    processServerCommand(cmdData);
  }
  return true;
}

bool syncTemplatesToRamCache(const String& remoteVer) {
  if (activeNet == NET_NONE || apiSecret.length() == 0) return false;

  if (remoteVer == "empty") {
    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      tplCache.clear();
      tplCache.shrink_to_fit();
      tplCacheSynced = true;
      tplCacheComplete = true;
      cachedTplVer = "empty";
      xSemaphoreGive(stateMutex);
    }
    Serial.println("[CACHE] Database template kosong (0 template di RAM).");
    return true;
  }

  if (ESP.getFreeHeap() < 80000 && stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
    tplCache.clear();
    tplCache.shrink_to_fit();
    tplCacheSynced = false;
    xSemaphoreGive(stateMutex);
  }

  if (netMutex && xSemaphoreTake(netMutex, pdMS_TO_TICKS(10000)) != pdTRUE) return false;

  bool reachedEnd = false;
  bool complete = true;
  std::vector<CachedTemplate> staging;

  if (syncClockIfNeeded()) {
    String body = "action=list_templates";

    auto fetchIntoStaging = [&](Client& client, unsigned long streamTimeoutMs) -> bool {
      writeSignedPostHeadersAndBody(client, body);
      unsigned long deadline = millis() + streamTimeoutMs;
      bool gotStatus = false;
      int httpCode = 0;
      bool isBody = false;
      String line = "";

      while ((long)(millis() - deadline) < 0 && readLineWithDeadline(client, line, deadline)) {
        line.trim();
        if (!isBody) {
          if (!gotStatus && line.startsWith("HTTP/")) {
            int sp = line.indexOf(' ');
            if (sp > 0) httpCode = line.substring(sp + 1).toInt();
            gotStatus = true;
          } else if (line.length() == 0) {
            isBody = true;
            if (httpCode != 200) {
              Serial.printf("[CACHE] list_templates HTTP %d\n", httpCode);
              client.stop();
              return false;
            }
          }
          continue;
        }
        if (line == "END") {
          client.stop();
          return true;
        }
        if (line.startsWith("ITEM:")) {
          int c1 = 5;
          int c2 = line.indexOf(':', c1);
          if (c2 > c1) {
            int slotId = line.substring(c1, c2).toInt();
            String hexTpl = line.substring(c2 + 1);
            if (slotId > 0 && hexTpl.length() >= 1024) {
              if (staging.size() < MAX_CACHED_TEMPLATES) {
                CachedTemplate entry;
                entry.slotId = (uint16_t)slotId;
                if (hexToBytes512(hexTpl, entry.data)) {
                  staging.push_back(entry);
                }
              } else {
                complete = false;
              }
            }
          }
        }
      }
      client.stop();
      return false;
    };

    if (activeNet == NET_WIFI) {
      WiFiClientSecure client;
      client.setCACert(ISRG_ROOT_X1_CA);
      client.setTimeout(6);
      client.setHandshakeTimeout(6);
      if (client.connect(serverHost.c_str(), wifiHttpsPort, 5000)) {
        reachedEnd = fetchIntoStaging(client, 12000);
      }
    } else if (activeNet == NET_LAN) {
      EthernetClient client;
      if (connectLanToServer(client, 1000)) {
        reachedEnd = fetchIntoStaging(client, 8000);
      }
    }
  }

  if (netMutex) xSemaphoreGive(netMutex);

  if (!reachedEnd) {
    Serial.println("[CACHE] Gagal sinkron template ke RAM, mencoba lagi pada ping berikutnya.");
    return false;
  }

  size_t count = staging.size();
  if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
    tplCache = std::move(staging);
    tplCacheSynced = true;
    tplCacheComplete = complete;
    cachedTplVer = remoteVer;
    xSemaphoreGive(stateMutex);
  }
  Serial.printf("[CACHE] Tersinkron %u template ke RAM (ver: %s, lengkap: %s, heap: %u)\n",
                (unsigned)count, remoteVer.c_str(), complete ? "YA" : "TIDAK", ESP.getFreeHeap());
  return true;
}

void netBgTask(void* param) {
  (void)param;
  unsigned long lastPingMs = 0;

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(150));

    if (setupPortalActive || bgPaused || activeNet == NET_NONE) {
      continue;
    }

    unsigned long now = millis();
    if (!bgForcePing && lastPingMs != 0 && (now - lastPingMs < 15000UL)) {
      continue;
    }

    bgForcePing = false;
    lastPingMs = now;

    if (apiSecret.length() == 0) {
      continue;
    }

    if (activeNet == NET_LAN && netMutex && xSemaphoreTake(netMutex, pdMS_TO_TICKS(300)) == pdTRUE) {
      Ethernet.maintain();
      xSemaphoreGive(netMutex);
    }

    String body = "action=ping&ip=" + currentIpString() + "&koneksi=" + String(currentNetName());
    Serial.printf("\n[BG:%s] POST %s (ping)...\n", currentNetName(), serverHost.c_str());

    String responseLine;
    int httpCode = 0;
    if (!postSignedApi(body, responseLine, httpCode, 4500)) {
      Serial.println(" -> [BG] Server belum merespon.");
      continue;
    }

    Serial.printf(" -> [BG] HTTP %d\n", httpCode);

    if (httpCode == 401) {
      if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        pendingAuthFail = true;
        hasPendingPing = true;
        xSemaphoreGive(stateMutex);
      }
      continue;
    }

    if (httpCode != 200 || responseLine.length() == 0) {
      continue;
    }

    String disp     = extractJsonString(responseLine, "display");
    String cmd      = extractJsonString(responseLine, "cmd");
    String fpMap    = extractJsonString(responseLine, "fp_map");
    String tplVer   = extractJsonString(responseLine, "tpl_ver");
    String tplSlots = extractJsonString(responseLine, "tpl_slots");
    disp.trim();
    cmd.trim();

    if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      pendingDisplay  = disp;
      pendingCmd      = cmd;
      pendingFpMap    = fpMap;
      pendingAuthFail = false;
      hasPendingPing  = true;
      xSemaphoreGive(stateMutex);
    }

    String remoteVer = tplVer.length() > 0 ? tplVer : tplSlots;
    if (remoteVer.length() == 0) remoteVer = "empty";

    bool needSync = bgForceCacheSync;
    if (!needSync && stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
      needSync = (!tplCacheSynced || remoteVer != cachedTplVer);
      xSemaphoreGive(stateMutex);
    }

    if (needSync && !bgPaused && !setupPortalActive) {
      bgForceCacheSync = false;
      syncTemplatesToRamCache(remoteVer);
    }
  }
}

// ==========================================
// HYBRID NETWORK MANAGER (PRIORITAS 1: LAN W5500, PRIORITAS 2: WI-FI HTTPS)
// ==========================================
void checkNetworkConnection() {
  if (!w5500Ok) {
    if (Ethernet.hardwareStatus() != EthernetNoHardware) {
      w5500Ok = true;
      Serial.println("\n[LAN] Modul W5500 terdeteksi aktif!");
    }
  }

  bool physicalLink = w5500Ok && (Ethernet.linkStatus() == LinkON);

  // Prioritas 1: Kabel LAN aktif
  if (physicalLink) {
    if (activeNet != NET_LAN) {
      if (netMutex) xSemaphoreTake(netMutex, pdMS_TO_TICKS(6000));
      if (WiFi.getMode() != WIFI_OFF) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        wifiConnecting = false;
      }

      Serial.println("\n[LAN] Kabel LAN terhubung! Mengambil IP...");
      showBigText("CONNECT LAN");

      if (Ethernet.begin(mac) == 0) {
        Serial.println(" -> DHCP timeout, menggunakan IP Statis 192.168.1.199");
        Ethernet.begin(mac, staticIp);
      }

      lanConnected = true;
      activeNet = NET_LAN;
      wifiFailCount = 0;
      lastWifiReason = 0;
      lastWifiError = "";
      initialPingSent = false;
      if (netMutex) xSemaphoreGive(netMutex);
      Serial.printf(" -> LAN Terhubung! IP ESP32: %s\n", Ethernet.localIP().toString().c_str());

      showBigText("LAN ONLINE");
      delay(400);
      showBigText(adminMessage);

      bgForceCacheSync = true;
      bgForcePing = true;
    }
    return;
  }

  // LAN terputus
  if (activeNet == NET_LAN || lanConnected) {
    lanConnected = false;
    activeNet = NET_NONE;
    initialPingSent = false;
    Serial.println("\n[LAN] Kabel LAN dicabut (Link Down).");
    showBigText("LAN PUTUS");
    delay(500);
    showBigText(adminMessage);
  }

  // Prioritas 2: Failover ke Wi-Fi
  if (savedSsid.length() == 0) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (activeNet != NET_WIFI) {
      wifiConnecting = false;
      wifiFailCount = 0;
      lastWifiReason = 0;
      lastWifiError = "";
      activeNet = NET_WIFI;
      initialPingSent = false;
      Serial.printf("\n[WIFI] Terhubung ke %s | IP: %s | RSSI: %d dBm\n",
                    savedSsid.c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
      showBigText("WIFI ONLINE");
      delay(400);
      showBigText(adminMessage);
      bgForceCacheSync = true;
      bgForcePing = true;
    }
  } else {
    if (activeNet == NET_WIFI) {
      activeNet = NET_NONE;
      initialPingSent = false;
      Serial.println("\n[WIFI] Koneksi Wi-Fi terputus.");
      showBigText("OFFLINE");
      delay(500);
      showBigText(adminMessage);
    }

    unsigned long now = millis();
    if (!wifiConnecting && (lastWifiAttempt == 0 || now - lastWifiAttempt >= 2000UL)) {
      lastWifiAttempt = now;
      wifiConnectStart = now;
      wifiConnecting = true;
      lastWifiReason = 0;
      WiFi.mode(WIFI_STA);
      WiFi.setTxPower(WIFI_POWER_15dBm);
      WiFi.setSleep(false);
      WiFi.setAutoReconnect(true);
      if (savedPass.length() > 0) {
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
      } else {
        WiFi.begin(savedSsid.c_str());
      }
      Serial.printf("\n[WIFI] Percobaan %u/2 ke SSID: %s...\n", (unsigned)(wifiFailCount + 1), savedSsid.c_str());
      showBigText("WIFI TRY " + String(wifiFailCount + 1));
    } else if (wifiConnecting && ((now - wifiConnectStart >= 10000UL) || (lastWifiReason == 201 && now - wifiConnectStart >= 4000UL))) {
      wifiConnecting = false;
      wifiFailCount++;
      lastWifiAttempt = now;
      if (lastWifiError.length() == 0) {
        lastWifiError = "Timeout koneksi ke Wi-Fi \"" + savedSsid + "\"";
      }
      Serial.printf("[WIFI] Gagal (%u/2): %s\n", (unsigned)wifiFailCount, lastWifiError.c_str());
      if (wifiFailCount >= 2) {
        wifiFailCount = 0;
        Serial.println("[WIFI] Gagal terhubung 2x, beralih ke Captive Portal Setup Wi-Fi...");
        startSetupPortal(false);
        return;
      }
    }
  }
}

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && (wifiConnecting || activeNet == NET_WIFI)) {
    uint8_t reason = info.wifi_sta_disconnected.reason;
    lastWifiReason = reason;
    if (reason == 201) {
      lastWifiError = "SSID tidak ditemukan (pastikan Wi-Fi 2.4 GHz, bukan 5 GHz)";
    } else if (reason == 2 || reason == 15 || reason == 202 || reason == 204) {
      lastWifiError = "Password Wi-Fi salah atau autentikasi ditolak (pastikan WPA2)";
    } else if (reason == 205) {
      lastWifiError = "Gagal terhubung ke Access Point";
    } else {
      lastWifiError = "Koneksi gagal (kode " + String(reason) + ")";
    }
  }
}

// ==========================================
// CAPTIVE PORTAL SETUP MODE (AP + DNS + WEB SERVER)
// ==========================================
String htmlEscape(const String& s) {
  String out;
  out.reserve(s.length() + 16);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else if (c == '\'') out += "&#39;";
    else out += c;
  }
  return out;
}

void setupPortalHandlers() {
  setupWebServer.on("/", HTTP_GET, []() {
    String html;
    html.reserve(3400);
    html  = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>Setup Wi-Fi Mesin Absensi</title>";
    html += "<style>";
    html += "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;max-width:460px;margin:16px auto;padding:14px;background:#f8fafc;color:#0f172a}";
    html += ".card{background:#fff;border:1px solid #e2e8f0;border-radius:12px;padding:16px;margin-bottom:14px;box-shadow:0 2px 8px rgba(0,0,0,0.04)}";
    html += "h2{margin:0 0 8px;font-size:18px;color:#047857}label{display:block;font-size:13px;font-weight:600;margin:10px 0 4px;color:#334155}";
    html += "input{width:100%;padding:10px;border:1.5px solid #cbd5e1;border-radius:8px;box-sizing:border-box;font-size:14px}";
    html += "button,.btn{width:100%;padding:11px;margin-top:12px;border:none;border-radius:8px;font-size:14px;font-weight:700;cursor:pointer;color:#fff;background:#059669}";
    html += ".btn-sec{background:#0284c7}.btn-gray{background:#64748b}.btn-red{background:#dc2626}";
    html += ".dev{font-family:monospace;font-size:16px;font-weight:700;background:#ecfdf5;color:#065f46;padding:8px 10px;border-radius:6px;border:1px solid #a7f3d0;text-align:center}";
    html += ".net-item{padding:8px 10px;border:1px solid #e2e8f0;border-radius:6px;margin-top:6px;display:flex;justify-content:space-between;cursor:pointer;background:#f8fafc;font-size:13px}";
    html += "</style></head><body>";

    html += "<div class='card'><h2>Setup Mesin Absensi Denanyar</h2>";
    html += "<div style='font-size:12px;color:#64748b;margin-bottom:6px'>Device ID (daftarkan di admin SIAKAD):</div>";
    html += "<div class='dev'>" + htmlEscape(deviceId) + "</div>";
    html += "<p style='font-size:12px;color:#475569;margin:8px 0 0'>SSID Tersimpan: <b>" + (savedSsid.length() ? htmlEscape(savedSsid) : String("-")) + "</b> &bull; Secret: <b>" + (apiSecret.length() ? String("Terpasang") : String("Belum")) + "</b></p>";
    if (lastWifiError.length() > 0) {
      html += "<div style='background:#fef2f2;border:1px solid #fecaca;color:#991b1b;padding:10px 12px;border-radius:8px;margin-top:10px;font-size:13px'>";
      html += "<b>Gagal konek ke \"" + htmlEscape(savedSsid) + "\":</b><br>" + htmlEscape(lastWifiError);
      html += "</div>";
    }
    html += "</div>";

    html += "<div class='card'>";
    html += "<button type='button' class='btn-sec' onclick='scanWifi()'>Scan Wi-Fi Terdekat (2.4 GHz)</button>";
    html += "<div style='font-size:11px;color:#64748b;margin-top:6px'>Catatan: ESP32 hanya mendukung Wi-Fi <b>2.4 GHz</b> (tidak bisa 5 GHz).</div>";
    html += "<div id='scanList' style='margin-top:8px;font-size:13px;color:#64748b'></div>";
    html += "<form method='POST' action='/save'>";
    html += "<label>SSID Wi-Fi</label><input id='ssidInput' name='ssid' value='" + htmlEscape(savedSsid) + "' placeholder='Nama Wi-Fi kantor/kampus' required>";
    html += "<label>Password Wi-Fi</label><input type='password' name='pass' placeholder='Kosongkan jika tetap pakai password lama'>";
    html += "<label style='display:flex;align-items:center;gap:8px;margin-top:8px;font-weight:500'><input type='checkbox' name='open_wifi' value='1' style='width:auto'> Wi-Fi tanpa password (Open)</label>";
    html += "<label>API Secret (64 Hex dari Admin)</label><input name='secret' placeholder='Kosongkan jika tetap pakai secret tersimpan' style='font-family:monospace'>";
    html += "<label>Server Host</label><input name='host' value='" + htmlEscape(serverHost) + "' required>";
    html += "<button type='submit'>Simpan &amp; Restart</button>";
    html += "</form></div>";

    html += "<div class='card' style='display:grid;grid-template-columns:1fr 1fr;gap:8px'>";
    html += "<form method='POST' action='/exit' style='margin:0'><button type='submit' class='btn-gray' style='margin-top:0'>Keluar Portal</button></form>";
    html += "<form method='POST' action='/reset' style='margin:0' onsubmit=\"return confirm('Hapus Wi-Fi tersimpan?')\"><button type='submit' class='btn-red' style='margin-top:0'>Hapus Wi-Fi</button></form>";
    html += "</div>";

    html += "<script>async function scanWifi(){const b=document.getElementById('scanList');b.textContent='Memindai Wi-Fi 2.4 GHz...';try{const r=await fetch('/scan');const j=await r.json();b.innerHTML='';if(!j.length){b.textContent='Tidak ada jaringan 2.4 GHz ditemukan.';return;}for(const n of j){const d=document.createElement('div');d.className='net-item';const s1=document.createElement('span');const b1=document.createElement('b');b1.textContent=n.ssid;s1.appendChild(b1);const s2=document.createElement('span');s2.textContent=n.rssi+' dBm';d.appendChild(s1);d.appendChild(s2);d.onclick=()=>{document.getElementById('ssidInput').value=n.ssid;};b.appendChild(d);}}catch(e){b.textContent='Gagal scan.';}}window.addEventListener('load',scanWifi);</script>";
    html += "</body></html>";

    setupWebServer.send(200, "text/html", html);
  });

  setupWebServer.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
      String s = WiFi.SSID(i);
      s.replace("\\", "\\\\");
      s.replace("\"", "\\\"");
      if (i > 0) json += ",";
      json += "{\"ssid\":\"" + s + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    WiFi.scanDelete();
    setupWebServer.send(200, "application/json", json);
  });

  setupWebServer.on("/save", HTTP_POST, []() {
    String newSsid  = setupWebServer.arg("ssid");
    String newPass  = setupWebServer.arg("pass");
    String openWifi = setupWebServer.arg("open_wifi");
    String newSec   = setupWebServer.arg("secret");
    String newHost  = setupWebServer.arg("host");

    newSsid.trim();
    newSec.trim();
    newSec.toLowerCase();
    newHost.trim();

    if (newSsid.length() == 0) {
      setupWebServer.send(400, "text/html", "<h3>SSID wajib diisi.</h3><a href='/'>Kembali</a>");
      return;
    }

    if (openWifi == "1") {
      newPass = "";
    } else if (newSsid == savedSsid && newPass.length() == 0) {
      newPass = savedPass;
    }

    netPrefs.putString("ssid", newSsid);
    netPrefs.putString("pass", newPass);
    if (newSec.length() > 0) {
      netPrefs.putString("secret", newSec);
    }
    if (newHost.length() > 0) {
      netPrefs.putString("host", newHost);
    }

    setupWebServer.send(200, "text/html",
      "<html><body style='font-family:sans-serif;text-align:center;padding:30px'><h3>Konfigurasi Tersimpan!</h3><p>Mesin sedang restart dan mencoba terhubung ke <b>" + htmlEscape(newSsid) + "</b>...</p></body></html>");
    delay(1200);
    ESP.restart();
  });

  setupWebServer.on("/reset", HTTP_POST, []() {
    netPrefs.remove("ssid");
    netPrefs.remove("pass");
    savedSsid = "";
    savedPass = "";
    lastWifiError = "";
    lastWifiReason = 0;
    wifiFailCount = 0;
    setupWebServer.send(200, "text/html",
      "<html><body style='font-family:sans-serif;text-align:center;padding:30px'><h3>Data Wi-Fi Dihapus!</h3><a href='/'>Kembali</a></body></html>");
  });

  setupWebServer.on("/exit", HTTP_POST, []() {
    setupWebServer.send(200, "text/html",
      "<html><body style='font-family:sans-serif;text-align:center;padding:30px'><h3>Keluar dari Setup Portal...</h3></body></html>");
    delay(400);
    stopSetupPortal();
  });

  setupWebServer.onNotFound([]() {
    setupWebServer.sendHeader("Location", "http://192.168.4.1/", true);
    setupWebServer.send(302, "text/plain", "");
  });
}

void startSetupPortal(bool manualTrigger) {
  if (setupPortalActive) return;
  bgPaused = true;
  if (netMutex && xSemaphoreTake(netMutex, pdMS_TO_TICKS(6000)) == pdTRUE) {
    xSemaphoreGive(netMutex);
  }
  wifiConnecting = false;
  setupPortalActive = true;
  setupPortalManual = manualTrigger;
  setupPortalStart = millis();
  portalIdleSince = millis();
  setupOledCycleAt = 0;
  setupOledPage = 0;
  activeNet = NET_NONE;
  lanConnected = false;

  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setTxPower(WIFI_POWER_15dBm);
  WiFi.softAP(apSsid.c_str(), "denanyar1234");
  delay(100);

  dnsServer.start(53, "*", WiFi.softAPIP());
  setupPortalHandlers();
  setupWebServer.begin();

  Serial.println("\n========================================");
  Serial.println("  MODE CAPTIVE PORTAL SETUP WI-FI");
  Serial.printf("  SSID AP   : %s\n", apSsid.c_str());
  Serial.println("  Password  : denanyar1234");
  Serial.printf("  URL Setup : http://%s/\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("  Device ID : %s\n", deviceId.c_str());
  Serial.println("========================================\n");

  showBigText(apSsid);
}

void stopSetupPortal() {
  if (!setupPortalActive) return;
  setupPortalActive = false;
  setupWebServer.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  lastWifiAttempt = 0;
  wifiFailCount = 0;
  bgPaused = false;
  bgForcePing = true;
  Serial.println("[PORTAL] Mode Setup Portal ditutup, kembali ke operasi normal.");
  showBigText(adminMessage);
}

void handleSetupPortal() {
  dnsServer.processNextRequest();
  setupWebServer.handleClient();

  unsigned long now = millis();
  if (WiFi.softAPgetStationNum() > 0) {
    portalIdleSince = now;
  }

  if (now - setupOledCycleAt >= 4000UL) {
    setupOledCycleAt = now;
    setupOledPage = (setupOledPage + 1) % 2;
    if (setupOledPage == 0) {
      showBigText("SETUP WIFI");
    } else {
      showBigText("192.168.4.1");
    }

    if (!setupPortalManual && w5500Ok && Ethernet.linkStatus() == LinkON) {
      Serial.println("[PORTAL] Kabel LAN terdeteksi, menutup Setup Portal...");
      stopSetupPortal();
      return;
    }
    if (!setupPortalManual && savedSsid.length() > 0 && WiFi.softAPgetStationNum() == 0 && (now - portalIdleSince >= 120000UL)) {
      Serial.println("[PORTAL] Tidak ada klien terhubung selama 2 menit, mencoba ulang Wi-Fi tersimpan...");
      stopSetupPortal();
      return;
    }
  }
}

// ==========================================
// SERIAL MONITOR CLI
// ==========================================
void printDeviceInfo() {
  size_t cacheCnt = 0;
  bool cacheReady = false;
  String verStr = "-";
  if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    cacheCnt = tplCache.size();
    cacheReady = tplCacheSynced;
    if (cachedTplVer.length() > 0) verStr = cachedTplVer;
    xSemaphoreGive(stateMutex);
  }

  Serial.println("\n--- INFO MESIN ABSENSI ---");
  Serial.printf("Device ID   : %s\n", deviceId.c_str());
  Serial.printf("AP SSID     : %s (Pass: denanyar1234)\n", apSsid.c_str());
  Serial.printf("Saved SSID  : %s\n", savedSsid.length() ? savedSsid.c_str() : "(belum diatur)");
  Serial.printf("Secret Set  : %s (%d chars)\n", apiSecret.length() ? "YES" : "NO", (int)apiSecret.length());
  Serial.printf("Server Host : %s\n", serverHost.c_str());
  Serial.printf("Active Net  : %s\n", currentNetName());
  Serial.printf("IP Address  : %s\n", currentIpString().c_str());
  Serial.printf("Clock UTC   : %lu (%s)\n", (unsigned long)time(nullptr), isClockValid() ? "VALID" : "BELUM SYNC");
  Serial.printf("Cache RAM   : %u template (%s, ver: %s)\n", (unsigned)cacheCnt, cacheReady ? "SYNC" : "BELUM", verStr.c_str());
  Serial.printf("Free Heap   : %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Buzz Mode   : %s\n", buzzMode.c_str());
  Serial.printf("Hardware    : OLED=%s | AS608=%s | W5500=%s | Portal=%s\n",
                oledOk ? "OK" : "ERR",
                fpOk ? "OK" : "ERR",
                w5500Ok ? "OK" : "ERR",
                setupPortalActive ? "ON" : "OFF");
  Serial.println("--------------------------\n");
}

void processSerialCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  String upper = cmd;
  upper.toUpperCase();

  if (upper == "INFO") {
    printDeviceInfo();
  } else if (upper == "HELP") {
    Serial.println("Perintah Serial: INFO | SCAN | BEEP | BUZZER <auto|active|passive> | WIFI <ssid>|<pass> | SECRET <64-hex> | HOST <domain> | SETUP | CLEARWIFI | REBOOT");
  } else if (upper == "BEEP") {
    Serial.printf("[BUZZER] Tes bunyi (mode: %s)...\n", buzzMode.c_str());
    Serial.println(" -> Tes 1: Nada sukses ganda (bip-bip)...");
    showBigText("TES SUKSES");
    beepBuzzer(45, 2);
    delay(450);

    Serial.println(" -> Tes 2: Nada gagal panjang (biiip)...");
    showBigText("TES GAGAL");
    beepBuzzer(150, 1);
    delay(400);

    showBigText(setupPortalActive ? "SETUP WIFI" : adminMessage);
    Serial.println("[BUZZER] Tes selesai.");
  } else if (upper.startsWith("BUZZER ")) {
    String m = cmd.substring(7);
    m.trim();
    m.toLowerCase();
    if (m != "auto" && m != "active" && m != "passive") {
      Serial.println("[CLI] Format salah. Gunakan: BUZZER auto | BUZZER active | BUZZER passive");
      return;
    }
    buzzMode = m;
    netPrefs.putString("buzz", buzzMode);
    Serial.printf("[CLI] Mode buzzer disimpan: %s\n", buzzMode.c_str());
    beepBuzzer(45, 1);
  } else if (upper == "SCAN") {
    WiFiMode_t prevMode = WiFi.getMode();
    if (prevMode == WIFI_OFF) {
      WiFi.mode(WIFI_STA);
      WiFi.setTxPower(WIFI_POWER_15dBm);
    }
    int n = WiFi.scanNetworks();
    Serial.printf("[SCAN] Ditemukan %d jaringan Wi-Fi:\n", n);
    for (int i = 0; i < n; i++) {
      Serial.printf("  %d) %s (%d dBm)\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
    WiFi.scanDelete();
    if (prevMode == WIFI_OFF && !setupPortalActive) WiFi.mode(WIFI_OFF);
  } else if (upper.startsWith("WIFI ")) {
    String arg = cmd.substring(5);
    int sep = arg.indexOf('|');
    if (sep <= 0) {
      Serial.println("[CLI] Format salah. Gunakan: WIFI <ssid>|<password>");
      return;
    }
    savedSsid = arg.substring(0, sep);
    savedSsid.trim();
    savedPass = arg.substring(sep + 1);
    netPrefs.putString("ssid", savedSsid);
    netPrefs.putString("pass", savedPass);
    wifiFailCount = 0;
    lastWifiReason = 0;
    lastWifiError = "";
    Serial.printf("[CLI] Wi-Fi disimpan: SSID=\"%s\".\n", savedSsid.c_str());
    if (setupPortalActive) {
      stopSetupPortal();
    }
    lastWifiAttempt = 0;
    wifiConnecting = false;
  } else if (upper.startsWith("SECRET ")) {
    String sec = cmd.substring(7);
    sec.trim();
    sec.toLowerCase();
    if (sec.length() != 64) {
      Serial.printf("[CLI] Peringatan: panjang secret %d karakter (standar 64 hex).\n", (int)sec.length());
    }
    bgPaused = true;
    if (netMutex && xSemaphoreTake(netMutex, pdMS_TO_TICKS(6000)) == pdTRUE) {
      xSemaphoreGive(netMutex);
    }
    apiSecret = sec;
    netPrefs.putString("secret", apiSecret);
    bgForceCacheSync = true;
    bgForcePing = true;
    bgPaused = false;
    Serial.printf("[CLI] API Secret disimpan (%d chars).\n", (int)apiSecret.length());
  } else if (upper.startsWith("HOST ")) {
    String h = cmd.substring(5);
    h.trim();
    if (h.length() > 0) {
      bgPaused = true;
      if (netMutex && xSemaphoreTake(netMutex, pdMS_TO_TICKS(6000)) == pdTRUE) {
        xSemaphoreGive(netMutex);
      }
      serverHost = h;
      netPrefs.putString("host", serverHost);
      cachedServerIp = IPAddress(0, 0, 0, 0);
      bgForceCacheSync = true;
      bgForcePing = true;
      bgPaused = false;
      Serial.printf("[CLI] Server Host diubah menjadi: %s\n", serverHost.c_str());
    }
  } else if (upper == "SETUP") {
    startSetupPortal(true);
  } else if (upper == "CLEARWIFI") {
    netPrefs.remove("ssid");
    netPrefs.remove("pass");
    savedSsid = "";
    savedPass = "";
    wifiFailCount = 0;
    lastWifiReason = 0;
    lastWifiError = "";
    Serial.println("[CLI] Kredensial Wi-Fi dihapus dari NVS.");
  } else if (upper == "REBOOT") {
    Serial.println("[CLI] Me-restart ESP32...");
    delay(200);
    ESP.restart();
  } else {
    Serial.printf("[CLI] Perintah tidak dikenal: %s (ketik HELP)\n", cmd.c_str());
  }
}

void handleSerialCli() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (serialLineBuf.length() > 0) {
        processSerialCommand(serialLineBuf);
        serialLineBuf = "";
      }
    } else if (serialLineBuf.length() < 240) {
      serialLineBuf += c;
    }
  }
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

  beepBuzzer(45, 1);
  showBigText("ANGKAT");
  delay(1000);
  while (finger.getImage() != FINGERPRINT_NOFINGER) {
    delay(40);
  }

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

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    setSensorStandby();
    beepBuzzer(200, 1);
    showBigText("BEDA JARI");
    delay(1500);
    showBigText(adminMessage);
    return false;
  }

  lastEnrolledTemplateHex = extractTemplateHexFromBuffer();
  setSensorStandby();
  if (lastEnrolledTemplateHex.length() >= 1024) {
    Serial.printf(" -> SUKSES! Pola jari ID #%d diekstrak dari RAM untuk dikirim ke Server (%d chars HEX).\n",
                  id, lastEnrolledTemplateHex.length());
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

// ==========================================
// SETUP & MAIN LOOP
// ==========================================
void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  Serial.begin(115200);
  delay(500);

  uint64_t chipMac = ESP.getEfuseMac();
  mac[0] = ((uint8_t)(chipMac & 0xFF) & 0xFE) | 0x02;
  mac[1] = (uint8_t)((chipMac >> 8) & 0xFF);
  mac[2] = (uint8_t)((chipMac >> 16) & 0xFF);
  mac[3] = (uint8_t)((chipMac >> 24) & 0xFF);
  mac[4] = (uint8_t)((chipMac >> 32) & 0xFF);
  mac[5] = (uint8_t)((chipMac >> 40) & 0xFF);

  char macHex[13];
  snprintf(macHex, sizeof(macHex), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  deviceId = String(macHex);
  shortId  = deviceId.substring(8);
  apSsid   = "ABSENSI-" + shortId;

  fpPrefs.begin("fp_names", false);
  fpTplPrefs.begin("fp_tpl", false);
  netPrefs.begin("net_cfg", false);

  savedSsid  = netPrefs.getString("ssid", "");
  savedPass  = netPrefs.getString("pass", "");
  apiSecret  = netPrefs.getString("secret", "");
  serverHost = netPrefs.getString("host", "denanyar.ac.id");
  if (serverHost.length() == 0) serverHost = "denanyar.ac.id";
  buzzMode   = netPrefs.getString("buzz", "auto");
  if (buzzMode != "auto" && buzzMode != "active" && buzzMode != "passive") buzzMode = "auto";

  Serial.println("\n\n========================================");
  Serial.println("  SISTEM ABSENSI HYBRID LAN+WIFI - ESP32");
  Serial.printf("  Device ID : %s\n", deviceId.c_str());
  Serial.println("========================================");

  netMutex = xSemaphoreCreateMutex();
  stateMutex = xSemaphoreCreateMutex();

  // 1. Inisialisasi OLED (128x32)
  Serial.println("\n[1] Menginisialisasi OLED 0.91\" (128x32)...");
  if (initOledDisplay(true)) {
    Serial.printf(" -> OLED 128x32 Berhasil Aktif (0x%02X)!\n", activeOledAddr);
    showBigText("MEMULAI");
  } else {
    Serial.println(" -> OLED tidak merespon di (D13/D14).");
  }

  // 2. Inisialisasi AS608 Fingerprint
  Serial.println("\n[2] Menginisialisasi AS608 Fingerprint...");
  pinMode(PIN_FP_TCH, INPUT);
  fpSerial.begin(57600, SERIAL_8N1, PIN_FP_RX, PIN_FP_TX);
  finger.begin(57600);
  delay(200);
  while (fpSerial.available()) fpSerial.read();

  if (finger.verifyPassword() && finger.getParameters() == FINGERPRINT_OK) {
    fpOk = true;
    Serial.printf(" -> Sensor Sidik Jari AS608 TERHUBUNG & OK! (Cap: %u)\n", finger.capacity);
    finger.emptyDatabase();
    fpTplPrefs.clear();
    Serial.println("    - Mode Murni Pembaca (Stateless): Memori Sensor Dikosongkan");
    setSensorStandby();
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

  SPI.end();
  SPI.begin(PIN_W5500_SCK, PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CS);
  Ethernet.init(PIN_W5500_CS);
  Ethernet.begin(mac, staticIp);
  SPI.begin(PIN_W5500_SCK, PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CS);

  auto hw = Ethernet.hardwareStatus();
  if (hw != EthernetNoHardware) {
    w5500Ok = true;
    Serial.printf(" -> Modul W5500 Terdeteksi OK! (Tipe: %d)\n", (int)hw);
  } else {
    Serial.println(" -> Modul W5500 Tidak Terdeteksi.");
  }

  // 4. Cek apakah pengguna menahan jari di sensor untuk memaksa masuk Setup Portal
  bool forceTouchPortal = false;
  if (digitalRead(PIN_FP_TCH) == HIGH) {
    Serial.println("[SETUP] Sentuhan terdeteksi saat boot, tahan ~2.5 detik untuk masuk Captive Portal...");
    showBigText("TAHAN SETUP");
    unsigned long tHold = millis();
    forceTouchPortal = true;
    while (millis() - tHold < 2500) {
      if (digitalRead(PIN_FP_TCH) == LOW) {
        forceTouchPortal = false;
        break;
      }
      delay(30);
    }
  }

  WiFi.onEvent(onWiFiEvent);

  bool hasLanNow = w5500Ok && (Ethernet.linkStatus() == LinkON);
  if (forceTouchPortal) {
    startSetupPortal(true);
  } else if (!hasLanNow && savedSsid.length() == 0) {
    startSetupPortal(false);
  } else {
    WiFi.mode(WIFI_OFF);
    checkNetworkConnection();
    setSensorStandby();
    if (!wifiConnecting && !setupPortalActive) {
      showBigText(adminMessage);
    }
  }

  beepBuzzer(45, 1);
  xTaskCreatePinnedToCore(netBgTask, "net_bg", 10240, nullptr, 1, nullptr, 0);
  printDeviceInfo();
  Serial.println("Ketik HELP di Serial Monitor untuk perintah konfigurasi.");
}

bool fingerHolding = false;
bool tchArmed = false;
unsigned long tchLowSince = 0;
unsigned long offlineTouchStart = 0;
unsigned long lastNetPoll = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastRecoverCheck = 0;

void loop() {
  unsigned long now = millis();

  // Layani Serial Monitor CLI di semua mode
  handleSerialCli();

  // Jika sedang di mode Captive Portal Setup Wi-Fi
  if (setupPortalActive) {
    handleSetupPortal();
    delay(5);
    return;
  }

  // Terapkan hasil ping dari background task Core 0 ke OLED / NVS / Enroll pada Core 1
  if (hasPendingPing && stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    bool localHas = hasPendingPing;
    bool localAuthFail = pendingAuthFail;
    String localDisp = pendingDisplay;
    String localCmd  = pendingCmd;
    String localMap  = pendingFpMap;
    hasPendingPing   = false;
    pendingAuthFail  = false;
    pendingDisplay   = "";
    pendingCmd       = "";
    pendingFpMap     = "";
    xSemaphoreGive(stateMutex);

    if (localHas) {
      if (localAuthFail) {
        if (millis() >= statusHoldUntil) {
          showBigText("AUTH GAGAL");
          statusHoldUntil = millis() + 2000;
          oledNeedsReset = true;
        }
      } else {
        applyFpMapCsv(localMap);
        if (localDisp.length() > 0 && localDisp != adminMessage) {
          adminMessage = localDisp;
          if (millis() >= statusHoldUntil) {
            showBigText(adminMessage);
          }
          Serial.printf("[SERVER] Teks layar standby diperbarui: \"%s\"\n", adminMessage.c_str());
        }
        if (localCmd.length() > 0) {
          processServerCommand(localCmd);
        }
      }
    }
  }

  // Pemulihan otomatis perangkat tiap 5 detik jika belum siap
  if ((!oledOk || !fpOk || !w5500Ok) && (now - lastRecoverCheck >= 5000)) {
    lastRecoverCheck = now;
    if (!oledOk) {
      if (initOledDisplay(false)) {
        Serial.printf("\n[OLED] Layar OLED 0x%02X pulih & aktif kembali!\n", activeOledAddr);
        showBigText(setupPortalActive ? "SETUP WIFI" : adminMessage);
      }
    }
    if (!fpOk) {
      while (fpSerial.available()) fpSerial.read();
      if (finger.verifyPassword() && finger.getParameters() == FINGERPRINT_OK) {
        fpOk = true;
        Serial.printf("\n[FP] Sensor AS608 aktif! (Cap: %u)\n", finger.capacity);
        finger.emptyDatabase();
        setSensorStandby();
      }
    }
    if (!w5500Ok) {
      digitalWrite(PIN_W5500_RST, LOW);
      delay(20);
      digitalWrite(PIN_W5500_RST, HIGH);
      delay(50);
      SPI.end();
      SPI.begin(PIN_W5500_SCK, PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CS);
      Ethernet.init(PIN_W5500_CS);
      Ethernet.begin(mac, staticIp);
      SPI.begin(PIN_W5500_SCK, PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CS);
      auto hw = Ethernet.hardwareStatus();
      if (hw != EthernetNoHardware) {
        w5500Ok = true;
        Serial.printf("\n[W5500] Modul W5500 aktif! (Tipe: %d)\n", (int)hw);
        checkNetworkConnection();
      }
    }
  }

  // 1. Cek status jaringan (LAN PoE + Wi-Fi Failover) tiap 1 detik
  if (now - lastNetPoll >= 1000) {
    lastNetPoll = now;
    checkNetworkConnection();
    if (setupPortalActive) return;
  }

  // 2. Sensor Sidik Jari (Responsif Cepat + Standby Lampu Mati + Anti-Pantulan TCH)
  int touch = digitalRead(PIN_FP_TCH);

  // Shortcut saat offline: tahan jari 5 detik untuk membuka Captive Portal Setup Wi-Fi
  if (touch == HIGH && activeNet == NET_NONE && !setupPortalActive && fpOk) {
    if (offlineTouchStart == 0) offlineTouchStart = now;
    if (now - offlineTouchStart >= 5000UL) {
      offlineTouchStart = 0;
      fingerHolding = false;
      setSensorStandby();
      Serial.println("[SETUP] Tahan jari 5 detik terdeteksi saat offline -> membuka Captive Portal...");
      startSetupPortal(true);
      return;
    }
  } else {
    offlineTouchStart = 0;
  }

  if (oledNeedsReset && !fingerHolding && now >= statusHoldUntil) {
    oledNeedsReset = false;
    showBigText(adminMessage);
  }

  if (touch == LOW) {
    if (tchLowSince == 0) tchLowSince = now;
    if (now >= ledOffCooldownUntil && (now - tchLowSince >= 80)) {
      tchArmed = true;
      if (fingerHolding) {
        fingerHolding = false;
        Serial.println("[FP] Jari diangkat. Siap scan berikutnya.");
      }
    }
  } else {
    tchLowSince = 0;
  }

  if (fpOk && touch == HIGH && tchArmed && !fingerHolding && now >= ledOffCooldownUntil) {
    unsigned long tHighStart = millis();
    while (digitalRead(PIN_FP_TCH) == HIGH && millis() - tHighStart < 20) delay(1);
    unsigned long duration = millis() - tHighStart;

    if (duration >= 18) {
      tchArmed = false;
      fingerHolding = true;
      finger.LEDcontrol(true);
      delay(25);

      uint8_t p = FINGERPRINT_NOFINGER;
      unsigned long tStart = millis();
      while (p != FINGERPRINT_OK && (millis() - tStart < 750)) {
        p = finger.getImage();
        if (p == FINGERPRINT_OK) break;
        if ((millis() - tStart >= 350) && (digitalRead(PIN_FP_TCH) == LOW)) break;
        delay(20);
      }

      if (p == FINGERPRINT_OK) {
        Serial.println("\n[FP] Sentuhan & jari nyata terdeteksi! Membaca sidik...");
        showBigText("MEMBACA...");
        p = finger.image2Tz(1);
        setSensorStandby();

        if (p == FINGERPRINT_OK) {
          uint16_t matchScore = 0;
          int matchedId = -1;
          bool hasCache = false;
          bool isComplete = false;
          if (stateMutex && xSemaphoreTake(stateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            hasCache = tplCacheSynced;
            isComplete = tplCacheComplete;
            xSemaphoreGive(stateMutex);
          }

          if (hasCache) {
            matchedId = matchFingerFromRamCache(matchScore);
          }
          if (!hasCache || (matchedId <= 0 && !isComplete)) {
            matchedId = matchFingerAgainstServer(matchScore);
          }

          if (matchedId > 0) {
            finger.fingerID = matchedId;
            finger.confidence = matchScore;
            lastScannedID = matchedId;
            String namaSantri = getSlotName(matchedId);
            String teksSukses = (namaSantri.length() > 0)
                                  ? ("SUKSES " + namaSantri)
                                  : ("SUKSES #" + String(matchedId));
            Serial.printf(" -> COCOK%s! ID #%d (%s, Akurasi: %d)\n",
                          hasCache ? " (RAM)" : " (SERVER)",
                          matchedId, teksSukses.c_str(), matchScore);

            bool serverHandled = false;
            if (activeNet != NET_NONE) {
              serverHandled = syncWithServerDenanyar("absen", matchedId);
            }
            if (!serverHandled) {
              lastDisplayedScan = teksSukses;
              beepBuzzer(45, 2);
              showBigText(teksSukses);
              statusHoldUntil = millis() + 1800;
            }
            oledNeedsReset = true;
          } else {
            Serial.println(" -> Foto sidik sukses, tetapi BELUM TERDAFTAR atau gagal hubungi server.");
            beepBuzzer(150, 1);
            showBigText("TDK TERDAFTAR");
            statusHoldUntil = millis() + 1500;
            oledNeedsReset = true;
          }
        } else {
          Serial.println(" -> Gagal konversi citra sidik.");
          beepBuzzer(150, 1);
          showBigText("ULANGI");
          statusHoldUntil = millis() + 1200;
          oledNeedsReset = true;
        }
      } else {
        setSensorStandby();
        fingerHolding = false;
      }
    }
  }

  // 3. Heartbeat status serial setiap 5 detik
  if (now - lastHeartbeat >= 5000) {
    lastHeartbeat = now;
    if (oledOk) {
      Wire.beginTransmission(activeOledAddr);
      uint8_t err = Wire.endTransmission();
      if (err != 0) {
        delay(10);
        Wire.beginTransmission(activeOledAddr);
        err = Wire.endTransmission();
      }
      if (err != 0) {
        Serial.printf("[OLED] Probe 0x%02X gagal (err=%u), mencoba pulihkan I2C...\n", activeOledAddr, err);
        if (initOledDisplay(false)) {
          Serial.printf("[OLED] Layar OLED 0x%02X berhasil dipulihkan!\n", activeOledAddr);
          showBigText(setupPortalActive ? "SETUP WIFI" : adminMessage);
        }
      }
    }
    Serial.printf("[HEARTBEAT] Up:%lus | Net:%s | IP:%s | OLED:%s | FP:%s | W5500:%s | TCH:%s | Pesan:%s\n",
                  now / 1000,
                  currentNetName(),
                  currentIpString().c_str(),
                  oledOk ? "OK" : "ERR",
                  fpOk ? "OK" : "ERR",
                  w5500Ok ? "OK" : "ERR",
                  digitalRead(PIN_FP_TCH) == HIGH ? "HIGH" : "LOW",
                  adminMessage.c_str());
  }

  delay(5);
}
