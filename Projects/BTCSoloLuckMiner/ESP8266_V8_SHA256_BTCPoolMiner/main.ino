// ------------------------------------------------------------
//  ESP8266 Stratum Miner for MAX on Mining-Dutch
//  - Bitcoin-style Stratum (mining.notify)
//  - Real merkle root (coinb1 + extranonce2 + coinb2 + merkle_branch[])
//  - Single SHA-256 over 80-byte header
// ------------------------------------------------------------

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <SHA256.h>

// Wi-Fi credentials
const char* ssid     = "TP-Link_CCC7";
const char* password = "69538001";

// Mining pool details (MAX on Mining-Dutch)
const char* pool_url  = "europe.mining-dutch.nl";
const int   pool_port = 6662;

// Stratum login details
const char* miner_username = "CFCbazar.worker2";
const char* miner_password = "d=0.0000028";
const char* worker_id      = "";

// Networking
WiFiClient client;

// Job data
String current_job_id   = "";
String prevhashHex      = "";
String coinb1Hex        = "";
String coinb2Hex        = "";
String versionHex       = "";
String bitsHex          = "";
String timeHex          = "";
String merkleBranches[16];
int    merkleBranchCount = 0;
bool   new_job_available = false;

// Target (from nBits)
uint8_t current_target[32];

// Merkle root for current job
uint8_t merkleRoot[32];
bool    merkleReady = false;

// extranonce2 (4 bytes)
uint32_t extranonce2 = 1;

// ------------------------------------------------------------
// Utility: hex string → bytes
// ------------------------------------------------------------
void hexToBytes(const String& hex, uint8_t* buffer, int len) {
  for (int i = 0; i < len; i++) {
    buffer[i] = strtoul(hex.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
  }
}

// ------------------------------------------------------------
// Utility: bytes → hex string
// ------------------------------------------------------------
String bytesToHex(const uint8_t* data, int len) {
  String out;
  out.reserve(len * 2);
  for (int i = 0; i < len; i++) {
    if (data[i] < 16) out += '0';
    out += String(data[i], HEX);
  }
  return out;
}

// ------------------------------------------------------------
// Reverse bytes in-place
// ------------------------------------------------------------
void reverseBytes(uint8_t* data, int len) {
  for (int i = 0; i < len / 2; i++) {
    uint8_t tmp = data[i];
    data[i] = data[len - 1 - i];
    data[len - 1 - i] = tmp;
  }
}

// ------------------------------------------------------------
// Compact nBits → 256-bit target (big-endian)
// ------------------------------------------------------------
void bitsToTarget(const String& bitsHex, uint8_t* target) {
  for (int i = 0; i < 32; i++) target[i] = 0;

  uint8_t bits[4];
  hexToBytes(bitsHex, bits, 4);

  uint8_t exponent = bits[0];
  uint32_t mantissa = (bits[1] << 16) | (bits[2] << 8) | bits[3];

  int offset = exponent - 3;
  if (offset <= 0 || offset > 32) return;

  target[32 - offset]     = (mantissa >> 16) & 0xFF;
  target[32 - offset + 1] = (mantissa >> 8)  & 0xFF;
  target[32 - offset + 2] = mantissa & 0xFF;
}

// ------------------------------------------------------------
// Single SHA-256
// ------------------------------------------------------------
void sha256Single(const uint8_t* data, size_t len, uint8_t* out) {
  SHA256 sha;
  sha.reset();
  sha.update(data, len);
  sha.finalize(out, 32);
}

// ------------------------------------------------------------
// Compare hash ≤ target (both big-endian)
// ------------------------------------------------------------
bool isHashValid(const uint8_t* hash, const uint8_t* target) {
  for (int i = 0; i < 32; i++) {
    if (hash[i] < target[i]) return true;
    if (hash[i] > target[i]) return false;
  }
  return false;
}

// ------------------------------------------------------------
// Build coinbase hash and merkle root
// ------------------------------------------------------------
void buildMerkleRoot() {
  char ex2[9];
  sprintf(ex2, "%08x", extranonce2);
  String extranonce2Hex = String(ex2);

  String coinbaseHex = coinb1Hex + extranonce2Hex + coinb2Hex;

  int coinbaseLen = coinbaseHex.length() / 2;
  uint8_t* coinbaseBytes = (uint8_t*)malloc(coinbaseLen);
  if (!coinbaseBytes) return;
  hexToBytes(coinbaseHex, coinbaseBytes, coinbaseLen);

  uint8_t curHash[32];
  sha256Single(coinbaseBytes, coinbaseLen, curHash);
  free(coinbaseBytes);

  for (int i = 0; i < merkleBranchCount; i++) {
    uint8_t branch[32];
    hexToBytes(merkleBranches[i], branch, 32);

    uint8_t buf[64];
    memcpy(buf, curHash, 32);
    memcpy(buf + 32, branch, 32);

    sha256Single(buf, 64, curHash);
  }

  memcpy(merkleRoot, curHash, 32);
  merkleReady = true;
}

// ------------------------------------------------------------
// Connect to pool
// ------------------------------------------------------------
void connectToMiningPool() {
  Serial.println("Connecting to pool...");
  if (client.connect(pool_url, pool_port)) {
    Serial.println("Connected to mining pool");
  } else {
    Serial.println("Pool connection failed");
    delay(5000);
    ESP.restart();
  }
}

// ------------------------------------------------------------
// Send login request
// ------------------------------------------------------------
void sendStratumLogin() {
  String loginRequest =
    String("{\"id\":1,\"method\":\"login\",\"params\":{") +
    "\"login\":\"" + miner_username + "\"," +
    "\"pass\":\""  + miner_password + "\"," +
    "\"worker_id\":\"" + worker_id + "\"}}\n";

  client.print(loginRequest);
  Serial.println("Sent login request");
}

// ------------------------------------------------------------
// Submit share (BTC-style mining.submit)
// ------------------------------------------------------------
void submitShare(const String& job_id,
                 const String& extranonce2Hex,
                 const String& nTimeHex,
                 const String& nonceHex) {
  String req = String("{\"id\":4,\"method\":\"mining.submit\",\"params\":[\"") +
               miner_username + "\",\"" +
               job_id + "\",\"" +
               extranonce2Hex + "\",\"" +
               nTimeHex + "\",\"" +
               nonceHex + "\"]}\n";
  client.print(req);
  Serial.println("Submitted share: " + req);
}

// ------------------------------------------------------------
// Handle incoming pool messages
// ------------------------------------------------------------
void handlePoolMessages(uint32_t &nonce) {
  while (client.available()) {
    String line = client.readStringUntil('\n');
    if (line.length() == 0) return;

    Serial.println("POOL: " + line);

    StaticJsonDocument<8192> doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err) return;

    if (doc["id"] == 1 && doc["result"].is<bool>() && doc["result"] == true) {
      Serial.println("Login OK");
    }

    if (doc["method"] == "mining.set_difficulty") {
      Serial.println("Difficulty update");
    }

    if (doc["method"] == "mining.notify") {
      JsonArray params = doc["params"].as<JsonArray>();
      if (params.size() < 9) return;

      current_job_id = params[0].as<String>();
      prevhashHex    = params[1].as<String>();
      coinb1Hex      = params[2].as<String>();
      coinb2Hex      = params[3].as<String>();

      merkleBranchCount = 0;
      JsonArray mb = params[4].as<JsonArray>();
      for (JsonVariant v : mb) {
        if (merkleBranchCount < 16) {
          merkleBranches[merkleBranchCount++] = v.as<String>();
        }
      }

      versionHex = params[5].as<String>();
      bitsHex    = params[6].as<String>();
      timeHex    = params[7].as<String>();
      bool clean_jobs = params[8].as<bool>();

      bitsToTarget(bitsHex, current_target);

      if (clean_jobs) {
        nonce = 0;
        extranonce2++;      // new job → new extranonce2
      } else {
        extranonce2++;      // still good to bump for uniqueness
      }

      buildMerkleRoot();

      new_job_available = true;
      Serial.println("New job received");
    }
  }
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  delay(200);

  Serial.println("\nBooting...");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");

  connectToMiningPool();
  sendStratumLogin();
}

// ------------------------------------------------------------
// Main mining loop
// ------------------------------------------------------------
void loop() {
  static uint32_t nonce = 0;

  handlePoolMessages(nonce);
  if (!new_job_available || !merkleReady) return;

  // Mine a batch of nonces per loop
  for (int i = 0; i < 512; i++) {
    uint8_t header[80];
    size_t offset = 0;

    // version (4 LE)
    hexToBytes(versionHex, header + offset, 4);
    offset += 4;

    // prevhash (32 LE: pool sends BE)
    hexToBytes(prevhashHex, header + offset, 32);
    reverseBytes(header + offset, 32);
    offset += 32;

    // merkle root (32 LE: we have BE)
    uint8_t merkleLE[32];
    memcpy(merkleLE, merkleRoot, 32);
    reverseBytes(merkleLE, 32);
    memcpy(header + offset, merkleLE, 32);
    offset += 32;

    // time (4 LE)
    hexToBytes(timeHex, header + offset, 4);
    offset += 4;

    // bits (4 LE)
    hexToBytes(bitsHex, header + offset, 4);
    offset += 4;

    // nonce (4 LE)
    header[76] = (uint8_t)(nonce & 0xFF);
    header[77] = (uint8_t)((nonce >> 8) & 0xFF);
    header[78] = (uint8_t)((nonce >> 16) & 0xFF);
    header[79] = (uint8_t)((nonce >> 24) & 0xFF);

    uint8_t hash[32];
    sha256Single(header, 80, hash);

    if (isHashValid(hash, current_target)) {
      Serial.println("🎯 Share candidate found!");
      Serial.print("Nonce: "); Serial.println(nonce);
      Serial.print("Hash:  "); Serial.println(bytesToHex(hash, 32));

      char ex2[9], nHex[9];
      sprintf(ex2, "%08x", extranonce2);
      sprintf(nHex, "%08x", nonce);

      submitShare(current_job_id, String(ex2), timeHex, String(nHex));
    }

    nonce++;
  }
}
