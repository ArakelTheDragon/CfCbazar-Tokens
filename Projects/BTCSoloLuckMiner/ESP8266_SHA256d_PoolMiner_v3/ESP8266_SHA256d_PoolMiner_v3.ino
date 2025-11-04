#include "BTC.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

// ─────────────────────────────────────────────
// 🔁 Helper: Reverse Hex String
// ─────────────────────────────────────────────
String reverseHex(const String& hex) {
  String reversed = "";
  for (int i = hex.length() - 2; i >= 0; i -= 2) {
    reversed += hex.substring(i, i + 2);
  }
  return reversed;
}

// ─────────────────────────────────────────────
// 📥 Persistent Merkle Branch Storage
// ─────────────────────────────────────────────
String merkleBranchList[12];  // Adjust size if needed
size_t merkleBranchCount = 0;

// ─────────────────────────────────────────────
// 📥 Parse Pool Response
// ─────────────────────────────────────────────
bool parsePoolResponse(String& jobId, String& versionHex, String& prevHashHex,
                       String& coinbase1, String& coinbase2,
                       String& timeHex, String& bitsHex) {
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

      DynamicJsonDocument doc(9999);
      DeserializationError err = deserializeJson(doc, line);
      if (err) {
        Serial.println("❌ JSON parse error");
        continue;
      }

      if (doc["method"] == "mining.notify") {
        JsonArray params = doc["params"];
        if (params.size() >= 9) {
          jobId        = params[0].as<String>();
          prevHashHex  = params[1].as<String>();
          coinbase1    = params[2].as<String>();
          coinbase2    = params[3].as<String>();

          JsonArray branches = params[4].as<JsonArray>();
          merkleBranchCount = 0;
          for (size_t i = 0; i < branches.size() && i < 12; i++) {
            merkleBranchList[i] = branches[i].as<String>();
            merkleBranchCount++;
          }

          versionHex   = reverseHex(params[5].as<String>());
          bitsHex      = params[6].as<String>();
          timeHex      = reverseHex(params[7].as<String>());
          return true;
        }
      }
    }
  }
  return false;
}

// ─────────────────────────────────────────────
// 🚀 Setup
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  delay(100);

  if (!LittleFS.begin()) {
    Serial.println("❌ LittleFS mount failed");
    return;
  }

  File logFile = LittleFS.open("/mining_log.txt", "r");
  if (logFile) {
    Serial.println("📄 Previous mining log:");
    while (logFile.available()) {
      Serial.write(logFile.read());
    }
    logFile.close();
  }

  connectToWiFi(ssid, password);
  connectToStratumPool(poolHost, poolPort);
  sendStratumLoginVerbose(minerUsername, minerPassword, workerId);
  readStratumResponse();

  String jobId, versionHex, prevHashHex, coinbase1, coinbase2, timeHex, bitsHex;

  if (!parsePoolResponse(jobId, versionHex, prevHashHex, coinbase1, coinbase2, timeHex, bitsHex)) {
    Serial.println("❌ Failed to receive job from pool");
    return;
  }

  static uint8_t target[32] __attribute__((aligned(4)));
  bitsToTarget(bitsHex.c_str(), target);

  Serial.println("🚀 Starting mining loop...");
  for (uint32_t nonce = 0; nonce < 100000; nonce++) {
    char nonceHexStr[9];
    sprintf(nonceHexStr, "%08x", nonce);

    String coinbaseHex = coinbase1 + String(nonceHexStr) + coinbase2;
    size_t coinbaseLen = coinbaseHex.length() / 2;

    if (coinbaseLen > sizeof(coinbaseBytes) || coinbaseHex.length() % 2 != 0) {
      Serial.println("⚠️ Skipping invalid coinbase");
      continue;
    }

    hexToBytes(coinbaseHex.c_str(), coinbaseBytes, coinbaseLen);
    sha256d(coinbaseBytes, coinbaseLen, coinbaseHash);
    Serial.print("🔍 coinbaseHash: ");
    Serial.println(toHex(coinbaseHash, 32));

    if (merkleBranchCount > 0) {
      for (size_t i = 0; i < merkleBranchCount; i++) {
        const char* branchHex = merkleBranchList[i].c_str();
        Serial.printf("🔍 Merkle branch [%d]: %s\n", i, branchHex);

        if (!branchHex || strlen(branchHex) < 64) {
          Serial.println("⚠️ Skipping invalid merkle branch");
          continue;
        }

        hexToBytes(branchHex, branchHash, 32);
        memcpy(combined, coinbaseHash, 32);
        memcpy(combined + 32, branchHash, 32);
        sha256d(combined, 64, coinbaseHash);
      }
    } else {
      Serial.println("⚠️ No merkle branches received");
    }

    reverseBytes(coinbaseHash, 32);
    String merkleHex = toHex(coinbaseHash, 32);

    buildBlockHeader(header, versionHex.c_str(), prevHashHex.c_str(), merkleHex.c_str(), timeHex.c_str(), bitsHex.c_str(), nonceHexStr);
    sha256d(header, 80, hash);

    Serial.print("Nonce: ");
    Serial.print(nonceHexStr);
    Serial.print(" → Hash: ");
    Serial.println(toHex(hash, 32));

    logMiningAttempt(nonceHexStr, header, hash);

    Serial.print("🎯 Target: ");
    Serial.println(toHex(target, 32));
    Serial.print("🔍 Final Hash: ");
    Serial.println(toHex(hash, 32));
    if (isHashValid(hash, target)) {
      Serial.println("🎯 Valid share found!");
      submitShare(jobId, nonceHexStr, timeHex, coinbaseHex);
      break;
    }
  }
}

void loop() {
  // Idle loop
}
