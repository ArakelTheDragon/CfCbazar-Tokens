#ifndef BTC_H
#define BTC_H

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <SHA256.h>
#include <LittleFS.h>

#define NONCE_FILE "/nonces.txt"


// ─────────────────────────────────────────────
// 🌐 Pool & WiFi Credentials
// ─────────────────────────────────────────────
const char* ssid = "YourSSID";
const char* password = "YourPW";

const char* poolHost = "eu3.solopool.org"; // Real mining pool, don't change unless you know what you are doing
const int poolPort = 8005; // Real port, don't change unless you know what you are doing
const char* minerUsername = "FBd767f6454bCd07c959da2E48fD429531A1323A"; // Your wallet
const char* minerPassword = "x"; // Don't change
const char* workerId = "esp_miner_01"; // Your worker ID


// ─────────────────────────────────────────────
// 🔧 Global Buffers (aligned for ESP8266)
// ─────────────────────────────────────────────
static uint8_t coinbaseBytes[256] __attribute__((aligned(4)));
static uint8_t coinbaseHash[32]   __attribute__((aligned(4)));
static uint8_t combined[64]       __attribute__((aligned(4)));
static uint8_t branchHash[32]     __attribute__((aligned(4)));
static uint8_t header[80]         __attribute__((aligned(4)));
static uint8_t hash[32]           __attribute__((aligned(4)));
static uint8_t alignedInputGlobal[256] __attribute__((aligned(4)));
//static uint8_t target[32] __attribute__((aligned(4)));
// ─────────────────────────────────────────────
// 🔧 Function Prototypes
// ─────────────────────────────────────────────

// Core Hashing
void sha256d(const uint8_t* data, size_t len, uint8_t* outHash);
String toHex(const uint8_t* data, size_t len);
void hexToBytes(const char* hex, uint8_t* bytes, size_t len);
void reverseBytes(uint8_t* data, size_t len);

// WorkToken Client
void connectToWiFi(const char* ssid, const char* password);
void setupDashboard(ESP8266WebServer& server, String& clientMAC, int& currentBlockNumber, String& currentJob, unsigned long& lastNonce, String& lastHashHex);
bool fetchJob(const char* serverHost, int& currentBlockNumber, String& messageOut);
bool submitNonce(const char* serverHost, const String& message, unsigned long nonce, const String& clientMAC, int currentBlockNumber);

// Luck Mining Nonce System
uint32_t generateUniqueNonce();
bool checkNonceExists(uint32_t nonce);
void storeNonce(uint32_t nonce);
void clearNonceFileIfTooLarge();
void resetNonceFile();
bool isHashBelowTarget(const uint8_t* hash, uint8_t thresholdZeros);

// Stratum Pool
void connectToStratumPool(const char* poolHost, int poolPort);
void sendStratumLogin(const char* minerUsername, const char* minerPassword, const char* workerId);
void sendStratumLoginVerbose(const char* minerUsername, const char* minerPassword, const char* workerId);
void readStratumResponse();


// ─────────────────────────────────────────────
// 🔧 Additional Function Prototypes
// ─────────────────────────────────────────────
void buildBlockHeader(uint8_t* header, const char* versionHex, const char* prevHashHex, const char* merkleHex, const char* timeHex, const char* bitsHex, const char* nonceHexStr);
void printHeaderHex(const uint8_t* header, size_t len);
void logMiningAttempt(const char* nonceHexStr, const uint8_t* header, const uint8_t* hash);
bool waitForStratumJob(String& jobId, String& versionHex, String& prevHashHex, String& coinbase1, String& coinbase2, JsonArray& merkleBranches, String& timeHex, String& bitsHex);
bool isHashValid(const uint8_t* hash, const uint8_t* target);
void bitsToTarget(const char* bitsHex, uint8_t* target);
void submitShare(const String& jobId, const char* nonceHexStr, const String& timeHex, const String& coinbaseHex);

// ─────────────────────────────────────────────
// 🔐 Core Hashing Utilities
// ─────────────────────────────────────────────
void sha256d(const uint8_t* data, size_t len, uint8_t* outHash) {
  SHA256 sha256;

  Serial.println("🔧 sha256d() called");
  Serial.printf("📍 data addr: %p, len: %d\n", data, len);
  Serial.printf("📍 outHash addr: %p\n", outHash);
  Serial.printf("📍 alignedInputGlobal addr: %p\n", alignedInputGlobal);

  if (len > sizeof(alignedInputGlobal)) {
    Serial.println("❌ sha256d: input too large");
    return;
  }

  memcpy(alignedInputGlobal, data, len);
  Serial.println("✅ sha256d: input copied to aligned buffer");

  Serial.print("🔍 alignedInput contents: ");
  for (size_t i = 0; i < len; i++) {
    if (alignedInputGlobal[i] < 0x10) Serial.print("0");
    Serial.print(alignedInputGlobal[i], HEX);
  }
  Serial.println();

  uint8_t firstHash[32] __attribute__((aligned(4)));
  Serial.printf("📍 firstHash addr: %p\n", firstHash);

  sha256.reset();
  Serial.println("✅ sha256.reset()");
  sha256.update(alignedInputGlobal, len);
  Serial.println("✅ sha256.update(alignedInputGlobal, len)");
  sha256.finalize(firstHash, sizeof(firstHash));
  Serial.println("✅ sha256.finalize(firstHash)");

  Serial.print("🔍 firstHash contents: ");
  for (size_t i = 0; i < 32; i++) {
    if (firstHash[i] < 0x10) Serial.print("0");
    Serial.print(firstHash[i], HEX);
  }
  Serial.println();

  sha256.reset();
  Serial.println("✅ sha256.reset()");
  sha256.update(firstHash, sizeof(firstHash));
  Serial.println("✅ sha256.update(firstHash)");
  sha256.finalize(outHash, sizeof(firstHash));
  Serial.println("✅ sha256.finalize(outHash)");

  Serial.print("🔍 outHash contents: ");
  for (size_t i = 0; i < 32; i++) {
    if (outHash[i] < 0x10) Serial.print("0");
    Serial.print(outHash[i], HEX);
  }
  Serial.println();
}

String toHex(const uint8_t* data, size_t len) {
  String hex = "";
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) hex += "0";
    hex += String(data[i], HEX);
  }
  return hex;
}

void hexToBytes(const char* hex, uint8_t* bytes, size_t len) {
  for (size_t i = 0; i < len; i++) {
    char buf[3] = { hex[i * 2], hex[i * 2 + 1], 0 };
    bytes[i] = strtoul(buf, nullptr, 16);
  }
}

void reverseBytes(uint8_t* data, size_t len) {
  for (size_t i = 0; i < len / 2; i++) {
    uint8_t temp = data[i];
    data[i] = data[len - 1 - i];
    data[len - 1 - i] = temp;
  }
}

// ─────────────────────────────────────────────
// 📡 WorkToken Client Utilities
// ─────────────────────────────────────────────
WiFiClient httpClient;

void connectToWiFi(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Remaining functions (setupDashboard, fetchJob, submitNonce, nonce system, stratum pool, block header, etc.) remain unchanged from your previous version unless you want me to refactor them for safety or performance.

void setupDashboard(ESP8266WebServer& server, String& clientMAC, int& currentBlockNumber, String& currentJob, unsigned long& lastNonce, String& lastHashHex) {
  server.on("/", HTTP_GET, [&]() {
    String html = "<html><head><title>WorkToken Miner</title></head><body>";
    html += "<h2>ESP8266 Miner Dashboard</h2>";
    html += "<p><b>WiFi:</b> " + WiFi.SSID() + "</p>";
    html += "<p><b>IP:</b> " + WiFi.localIP().toString() + "</p>";
    html += "<p><b>MAC:</b> " + clientMAC + "</p>";
    html += "<p><b>Block #:</b> " + String(currentBlockNumber) + "</p>";
    html += "<p><b>Job:</b> " + currentJob + "</p>";
    html += "<p><b>Last Nonce:</b> " + String(lastNonce) + "</p>";
    html += "<p><b>Last Hash:</b> " + lastHashHex + "</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.begin();
  Serial.println("Client dashboard started");
}

bool fetchJob(const char* serverHost, int& currentBlockNumber, String& messageOut) {
  HTTPClient http;
  String url = "http://" + String(serverHost) + "/getjob";
  http.begin(httpClient, url);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Job received:");
    Serial.println(payload);

    DynamicJsonDocument doc(512);
    deserializeJson(doc, payload);
    messageOut = doc["message"].as<String>();
    currentBlockNumber = doc["block_number"].as<int>();
    http.end();
    return true;
  } else {
    Serial.print("Failed to fetch job. HTTP code: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

bool submitNonce(const char* serverHost, const String& message, unsigned long nonce, const String& clientMAC, int currentBlockNumber) {
  HTTPClient http;
  String url = "http://" + String(serverHost) + "/submit";
  http.begin(httpClient, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "message=" + message +
                    "&nonce=" + String(nonce) +
                    "&mac=" + clientMAC +
                    "&block_number=" + String(currentBlockNumber);

  int httpCode = http.POST(postData);

  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("✅ Block accepted:");
    Serial.println(response);
    http.end();
    return true;
  } else {
    Serial.println("❌ Block rejected:");
    Serial.println(http.getString());
    http.end();
    return false;
  }
}

// ─────────────────────────────────────────────
// 🎲 Luck Mining Nonce System
// ─────────────────────────────────────────────

uint32_t generateUniqueNonce() {
  uint32_t nonce;
  bool isDuplicate = true;

  while (isDuplicate) {
    nonce = random(0xFFFFFFFF);
    isDuplicate = checkNonceExists(nonce);
  }

  storeNonce(nonce);
  return nonce;
}

bool checkNonceExists(uint32_t nonce) {
  File file = LittleFS.open(NONCE_FILE, "r");
  if (!file) return false;

  String target = String(nonce, HEX);
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.equalsIgnoreCase(target)) {
      file.close();
      return true;
    }
  }

  file.close();
  return false;
}

void storeNonce(uint32_t nonce) {
  File file = LittleFS.open(NONCE_FILE, "a");
  if (!file) {
    Serial.println("❌ Failed to open nonce file for writing");
    return;
  }

  file.println(String(nonce, HEX));
  file.close();
}

void clearNonceFileIfTooLarge() {
  File file = LittleFS.open(NONCE_FILE, "r");
  if (file && file.size() > 1000000) {
    file.close();
    LittleFS.remove(NONCE_FILE);
    Serial.println("🧹 Nonce file cleared (size limit exceeded)");
  } else {
    file.close();
  }
}

void resetNonceFile() {
  LittleFS.remove(NONCE_FILE);
  Serial.println("🧹 Nonce file manually reset");
}

bool isHashBelowTarget(const uint8_t* hash, uint8_t thresholdZeros) {
  for (uint8_t i = 0; i < thresholdZeros; i++) {
    if (hash[i] != 0x00) return false;
  }
  return true;
}

// ─────────────────────────────────────────────
// 🔌 Stratum Pool Connection
// ─────────────────────────────────────────────

WiFiClient stratumClient;

void connectToStratumPool(const char* poolHost, int poolPort) {
  Serial.print("Connecting to pool ");
  Serial.print(poolHost);
  Serial.print(":");
  Serial.println(poolPort);

  if (stratumClient.connect(poolHost, poolPort)) {
    Serial.println("✅ Connected to mining pool");
  } else {
    Serial.println("❌ Failed to connect to pool");
  }
}

void sendStratumLogin(const char* minerUsername, const char* minerPassword, const char* workerId) {
  String loginRequest = String("{\"id\": 1, \"method\": \"login\", \"params\": {\"login\": \"") +
                        minerUsername + "\", \"pass\": \"" + minerPassword +
                        "\", \"worker_id\": \"" + workerId + "\"}}\n";

  stratumClient.print(loginRequest);
  Serial.println("📤 Sent login request:");
  Serial.println(loginRequest);
}

void readStratumResponse() {
  while (stratumClient.available()) {
    String line = stratumClient.readStringUntil('\n');
    Serial.println("📥 Pool response:");
    Serial.println(line);
    stratumClient.print(line);

  }
}

// ─────────────────────────────────────────────
// 🪪 Stratum Login (Verbose)
// ─────────────────────────────────────────────
void sendStratumLoginVerbose(const char* minerUsername, const char* minerPassword, const char* workerId) {
  String loginRequest = String("{\"id\": 1, \"method\": \"login\", \"params\": {\"login\": \"") +
                        minerUsername + "\", \"pass\": \"" + minerPassword +
                        "\", \"worker_id\": \"" + workerId + "\"}}\n";

  Serial.println("📡 Sending Stratum login:");
  Serial.println(loginRequest);

  while (stratumClient.available()) {
    char ch = static_cast<char>(stratumClient.read());
    Serial.print(ch);
    if(!stratumClient.available()){
       Serial.println("Client not available in sendStratumLoginVerbose!");
    }
  }
  stratumClient.print(loginRequest);
}

// ─────────────────────────────────────────────
// 🧱 Build Full Block Header
// ─────────────────────────────────────────────
void buildBlockHeader(uint8_t* header, const char* versionHex, const char* prevHashHex, const char* merkleHex, const char* timeHex, const char* bitsHex, const char* nonceHexStr) {
  size_t offset = 0;
  hexToBytes(versionHex, header + offset, 4); offset += 4;
  hexToBytes(prevHashHex, header + offset, 32); offset += 32;
  hexToBytes(merkleHex, header + offset, 32); offset += 32;
  hexToBytes(timeHex, header + offset, 4); offset += 4;
  hexToBytes(bitsHex, header + offset, 4); offset += 4;
  hexToBytes(nonceHexStr, header + offset, 4); offset += 4;
}

// ─────────────────────────────────────────────
// 📥 Wait for Pool Response and Parse Job
// ─────────────────────────────────────────────
bool waitForStratumJob(String& jobId, String& versionHex, String& prevHashHex, String& coinbase1, String& coinbase2, JsonArray& merkleBranches, String& timeHex, String& bitsHex) {
  unsigned long start = millis();
  while (millis() - start < 30000) {
    if (stratumClient.available()) {
      String line = stratumClient.readStringUntil('\n');
      Serial.println("📥 Pool response:");
      Serial.println(line);

      File logFile = LittleFS.open("/mining_log.txt", "a");
      if (logFile) {
        logFile.println("Pool response:");
        logFile.println(line);
        logFile.close();
      }

      DynamicJsonDocument doc(2048);
      DeserializationError err = deserializeJson(doc, line);
      if (err) return false;

      if (doc["method"] == "mining.notify") {
        JsonArray params = doc["params"];
        if (params.size() >= 9) {
          jobId = params[0].as<String>();
          prevHashHex = params[1].as<String>();
          coinbase1 = params[2].as<String>();
          coinbase2 = params[3].as<String>();
          merkleBranches = params[4].as<JsonArray>();
          versionHex = params[5].as<String>();
          bitsHex = params[6].as<String>();
          timeHex = params[7].as<String>();
          return true;
        }
      }
    }
  }
  return false;
}

// ─────────────────────────────────────────────
// 📝 Log Mining Attempt to LittleFS
// ─────────────────────────────────────────────
void logMiningAttempt(const char* nonceHexStr, const uint8_t* header, const uint8_t* hash) {
  File logFile = LittleFS.open("/mining_log.txt", "a");
  if (!logFile) return;

  logFile.print("Nonce: 0x");
  logFile.println(nonceHexStr);

  logFile.print("Header: ");
  for (int i = 0; i < 80; i++) {
    if (header[i] < 0x10) logFile.print("0");
    logFile.print(header[i], HEX);
  }
  logFile.println();

  logFile.print("Hash: ");
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 0x10) logFile.print("0");
    logFile.print(hash[i], HEX);
  }
  logFile.println();
  logFile.println();

  logFile.close();
}

// ─────────────────────────────────────────────
// 📤 Submit Valid Share to Pool
// ─────────────────────────────────────────────
void submitShare(const String& jobId, const char* nonceHexStr, const String& timeHex, const String& coinbaseHex) {
  String submitRequest = String("{\"id\": 2, \"method\": \"mining.submit\", \"params\": [\"") +
                         workerId + "\", \"" + jobId + "\", \"" + nonceHexStr + "\", \"" +
                         timeHex + "\", \"" + coinbaseHex + "\"]}\n";

  Serial.println("📤 Submitting valid share to pool:");
  Serial.println(submitRequest);

  stratumClient.print(submitRequest);
}

bool isHashValid(const uint8_t* hash, const uint8_t* target) {
  if (!hash || !target) {
    Serial.println("❌ isHashValid: null pointer detected");
    return false;
  }

  Serial.print("🔍 hash: ");
  Serial.println(toHex(hash, 32));
  Serial.print("🔍 target: ");
  Serial.println(toHex(target, 32));

  return memcmp(hash, target, 32) < 0;
}

void bitsToTarget(const char* bitsHex, uint8_t* target) {
  uint32_t bits;
  sscanf(bitsHex, "%x", &bits);

  uint32_t exponent = bits >> 24;
  uint32_t mantissa = bits & 0xFFFFFF;

  memset(target, 0, 32);
  if (exponent <= 3) {
    mantissa >>= 8 * (3 - exponent);
    memcpy(target + 29, &mantissa, 3);
  } else {
    int offset = exponent - 3;
    memcpy(target + (32 - offset), &mantissa, 3);
  }
}

#endif

