We have made a working solo miner with ESP8266. Alpha testing complted and it seems to work. Now we need someone else to run it as well so we can be sure.

***Always use the latest version!***
**Pool connect format:**

const char* poolHost = "eu3.solopool.org";
const int poolPort = 8005;

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

**Pool login format and read response from pool:**
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

**SHA256D hash format:**
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

**Submit to pool format:**
void submitShare(const String& jobId, const char* nonceHexStr, const String& timeHex, const String& coinbaseHex) {
  String submitRequest = String("{\"id\": 2, \"method\": \"mining.submit\", \"params\": [\"") +
                         workerId + "\", \"" + jobId + "\", \"" + nonceHexStr + "\", \"" +
                         timeHex + "\", \"" + coinbaseHex + "\"]}\n";

  Serial.println("📤 Submitting valid share to pool:");
  Serial.println(submitRequest);

  stratumClient.print(submitRequest);
}

// Nonce gen block

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
