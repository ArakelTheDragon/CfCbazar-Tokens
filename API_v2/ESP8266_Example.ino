#include "reusable.h"

unsigned long lastApiCheck = 0;
const unsigned long apiCheckInterval = 60000;  // 1 minute

void setup() {
  setupExtender();
  Serial.println("setupExtender() ok");
}

void loop() {
  maintainSTAConnection();
  Serial.println("maintainSTAConnection() ok");

  // Periodically fetch token status from CfCbazar API
  if (millis() - lastApiCheck > apiCheckInterval) {
    fetchCfCbazarStatus();
    lastApiCheck = millis();
  }

  // Attempt to mine WorkTHR every 5 seconds
  mineWorkTHR();

  delay(1000);
}
