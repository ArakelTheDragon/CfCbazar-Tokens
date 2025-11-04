#ifndef BTC_H
#define BTC_H

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <SHA256.h>

const char* ssid = "TP-Link_CCC78";
const char* password = "69538001";

const char* poolHost = "eu3.solopool.org";
const int poolPort = 8005;
const char* minerUsername = "FBd767f6454bCd07c959da2E48fD429531A1323A";
const char* minerPassword = "x";
const char* workerId = "esp_miner_02";

// 80 bytes header parts
String versionHex   = "00804021";  // Version: 0x21408000 // Needs reversal
String prevHashHex  = "adfba2b408e45f0b37a7f0860070b66c35608ad38fc800000000000000000000";  // Already little-endian
String merkleHex    = "25a331ef9c4cfc2828fe5b810f6347011d0c348a8d058e73150feeac2d528eef";  // Needs reversal
String timeHex      = "e7ec0469";  // Timestamp: 0x6904ece7 // Needs reversal
String bitsHex      = "fbcd0117";  // Bits: 0x1701cdfb // Needs reversal
String nonceHex     = "670d3b5a";  // Nonce: 0x5a3b0d67 // Needs reversal
String jobId;  // Global to store current pool response job ID
String coinbaseHex;  // Store full coinbase for share submissions

// 🔧 Global Buffers (aligned for ESP8266)
static uint8_t coinbaseBytes[256] __attribute__((aligned(4)));
static uint8_t coinbaseHash[32]   __attribute__((aligned(4)));
static uint8_t branchHash[32]     __attribute__((aligned(4)));
static uint8_t combined[64]       __attribute__((aligned(4)));
static uint8_t header[80]         __attribute__((aligned(4)));
static uint8_t hash[32]           __attribute__((aligned(4)));



// Prototypes
void connectToWiFi(const char* ssid, const char* password);
void connectToStratumPool(const char* poolHost, int poolPort);
void sendStratumLogin(const char* minerUsername, const char* minerPassword, const char* workerId);
void sha256d(const uint8_t* data, size_t len, uint8_t* outHash);
String toHex(const uint8_t* data, size_t len);
void hexToBytes(const char* hex, uint8_t* bytes, size_t len);
void reverseBytes(uint8_t* data, size_t len);
void setupDashboard(ESP8266WebServer& server, String& clientMAC, int& currentBlockNumber, String& currentJob, unsigned long& lastNonce, String& lastHashHex);
bool fetchJob(const char* serverHost, int& currentBlockNumber, String& messageOut);
bool submitNonce(const char* serverHost, const String& message, unsigned long nonce, const String& clientMAC, int currentBlockNumber);
void bitsToTarget(const char* bitsHex, uint8_t* target);
bool isHashValid(const uint8_t* hash, const uint8_t* target);
void submitShare(const String& jobId, const char* nonceHexStr, const String& timeHex, const String& coinbaseHex);


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

  // ─────────────────────────────────────────────
  // 📤 Send login request
  // ─────────────────────────────────────────────
  stratumClient.print(loginRequest);
  Serial.println("📤 Sent login request:");
  Serial.println(loginRequest);

  // ─────────────────────────────────────────────
  // ⏳ Wait for pool response (with 50s timeout)
  // ─────────────────────────────────────────────
  const unsigned long timeoutMs = 50000;
  unsigned long start = millis();
  String response = "";
  String responseLines[10];
  size_t lineCount = 0;

  Serial.println("⏳ Waiting for pool response...");

  while (millis() - start < timeoutMs) {
    while (stratumClient.available()) {
      char c = stratumClient.read();
      response += c;
      if (c == '\n') {
        if (lineCount < 10) responseLines[lineCount++] = response;
        Serial.print("📥 ");
        Serial.print(response);

        // ─────────────────────────────────────────────
        // 📦 Parse mining.notify from pool
        // ─────────────────────────────────────────────
        DynamicJsonDocument doc(4096);
        DeserializationError err = deserializeJson(doc, response);
        if (!err && doc["method"] == "mining.notify") {
          JsonArray params = doc["params"];

          // Update globals
          jobId = params[0].as<String>();  // Store job ID          
          versionHex  = params[5].as<String>();
          prevHashHex = params[1].as<String>();

          // Build coinbase hash + merkle root
          String coinbase1 = params[2].as<String>();
          String coinbase2 = params[3].as<String>();
          JsonArray branches = params[4].as<JsonArray>();

          /*String*/ coinbaseHex = coinbase1 + "00000000" + coinbase2;  // placeholder nonce
          hexToBytes(coinbaseHex.c_str(), coinbaseBytes, coinbaseHex.length()/2);
          sha256d(coinbaseBytes, coinbaseHex.length()/2, coinbaseHash);

          for (JsonVariant branch : branches) {
            hexToBytes(branch.as<String>().c_str(), branchHash, 32);
            memcpy(combined, coinbaseHash, 32);
            memcpy(combined + 32, branchHash, 32);
            sha256d(combined, 64, coinbaseHash);
          }

          reverseBytes(coinbaseHash, 32);
          merkleHex = toHex(coinbaseHash, 32);

          timeHex = params[7].as<String>();
          bitsHex = params[6].as<String>();
          nonceHex = "00000000";  // will try during mining

          Serial.println("✅ Pool job recorded:");
          Serial.println("Version: " + versionHex);
          Serial.println("PrevHash: " + prevHashHex);
          Serial.println("Merkle: " + merkleHex);
          Serial.println("Time: " + timeHex);
          Serial.println("Bits: " + bitsHex);
        }

        response = "";
      }
    }

    if (lineCount > 0) break;  // stop waiting once we got a line
    delay(50);
  }

  if (lineCount == 0) {
    Serial.println("❌ No response from pool (timeout).");
  } else {
    Serial.println("📦 Recorded pool responses:");
    for (size_t i = 0; i < lineCount; i++) {
      Serial.printf("[%d] %s\n", i, responseLines[i].c_str());
    }
  }
}


// ─────────────────────────────────────────────
// 🔐 Core Hashing Utilities
// ─────────────────────────────────────────────

// SHA256d: Double SHA256 as used in Bitcoin
void sha256d(const uint8_t* data, size_t len, uint8_t* outHash) {
  SHA256 sha256;
  uint8_t firstHash[32];

  sha256.reset();
  sha256.update(data, len);
  sha256.finalize(firstHash, sizeof(firstHash));

  sha256.reset();
  sha256.update(firstHash, sizeof(firstHash));
  sha256.finalize(outHash, sizeof(firstHash));
}

// Convert hash to hex string
String toHex(const uint8_t* data, size_t len) {
  String hex = "";
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) hex += "0";
    hex += String(data[i], HEX);
  }
  return hex;
}

// Convert hex string to byte array
void hexToBytes(const char* hex, uint8_t* bytes, size_t len) {
  for (size_t i = 0; i < len; i++) {
    char buf[3] = { hex[i * 2], hex[i * 2 + 1], 0 };
    bytes[i] = strtoul(buf, nullptr, 16);
  }
}

// Reverse byte order (for little-endian formatting)
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
// Setup local dashboard
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

// Fetch job from server
bool fetchJob(const char* serverHost, int& currentBlockNumber, String& messageOut) {
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + String(serverHost) + "/getjob";
  http.begin(client, url);

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

// Submit mined block
bool submitNonce(const char* serverHost, const String& message, unsigned long nonce, const String& clientMAC, int currentBlockNumber) {
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + String(serverHost) + "/submit";
  http.begin(client, url);
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

void submitShare(const String& jobId, const char* nonceHexStr, const String& timeHex, const String& coinbaseHex) {
  String submitRequest = String("{\"id\": 2, \"method\": \"mining.submit\", \"params\": [\"") +
                         workerId + "\", \"" + jobId + "\", \"" + nonceHexStr + "\", \"" +
                         timeHex + "\", \"" + coinbaseHex + "\"]}\n";

  Serial.println("📤 Submitting valid share to pool:");
  Serial.println(submitRequest);

  stratumClient.print(submitRequest);
}

#endif
