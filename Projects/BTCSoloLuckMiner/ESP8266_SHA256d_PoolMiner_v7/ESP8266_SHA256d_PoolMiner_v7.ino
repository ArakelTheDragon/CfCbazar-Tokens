#include "BTC.h" // All functions, global variables and definitions

void setup() {
  // Start serial
  Serial.begin(9600);
  Serial.println("⛏️ ESP8266 Miner Starting...");

  // Start LittleFS
  if (!LittleFS.begin()) {
    Serial.println("❌ LittleFS mount failed");
    return;
  }
  
  // Seed random generator using analog noise
  randomSeed(analogRead(A0));

  // Clear nonce file if too large
  clearNonceFileIfTooLarge();
}

void loop() {
  // 1️⃣ Connect to Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n✅ Connected to Wi-Fi");
  }

  // 2️⃣ Connect to pool
  if (!stratumClient.connected()) {
    connectToStratumPool(poolHost, poolPort);
    while (!stratumClient.connected()) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n✅ Connected to mining pool");

    // 3️⃣ Send login to pool
    sendStratumLogin(minerUsername, minerPassword, workerId);

    // 4️⃣ Prepare header (little-endian formatting), reversing and hex conversion
    size_t offset = 0;
    hexToBytes(versionHex.c_str(), header + offset, 4); offset += 4;
    hexToBytes(prevHashHex.c_str(), header + offset, 32); offset += 32;  // DO NOT reverse
    hexToBytes(merkleHex.c_str(), header + offset, 32); reverseBytes(header + offset, 32); offset += 32;
    hexToBytes(timeHex.c_str(), header + offset, 4); offset += 4;
    hexToBytes(bitsHex.c_str(), header + offset, 4); offset += 4;
    // Nonce will be inserted dynamically
  }

  // 5️⃣ Convert bits to target once
  uint8_t target[32];
  bitsToTarget(bitsHex.c_str(), target);

  // 6️⃣ Mining loop while pool connection is active, gen nonce and hash
  while (stratumClient.connected()) {
    // Generate a unique random nonce
    uint32_t nonce = generateUniqueNonce();

    // Update nonce in header
    header[76] = nonce & 0xFF;
    header[77] = (nonce >> 8) & 0xFF;
    header[78] = (nonce >> 16) & 0xFF;
    header[79] = (nonce >> 24) & 0xFF;

    // SHA256d the header
    sha256d(header, 80, hash);

    // Check if hash meets target
    if (isHashValid(hash, target)) {
      Serial.println("🎯 Valid hash found!");
      Serial.print("Nonce: "); Serial.println(nonce);
      Serial.print("Hash: "); Serial.println(toHex(hash, 32));

      // Prepare nonce hex string for submission
      char nonceHexStr[9];
      sprintf(nonceHexStr, "%08x", nonce);

      // Submit the share using current job info
      submitShare(jobId, nonceHexStr, timeHex, coinbaseHex);

      // Break to fetch new job from pool
      break;
    }

    Serial.print("Nonce: ");
    Serial.println(nonce);

    vHandleSerialCommands();

    // Small delay to avoid watchdog reset on ESP8266
    delay(1);
    // Print the nonce file on demand by typing 'p' in the serial monitor
    if (Serial.read() == 'p') {
      printNonceFileContents();
    }
  }

  // If pool disconnects, loop will reconnect automatically
  // If pool disconnects nonce file contents will be printed
  printNonceFileContents();
}
