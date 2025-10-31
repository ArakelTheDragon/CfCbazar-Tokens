#include <LittleFS.h>
#include "WorkTokenServer.h"

void setup(void) {
  Serial.begin(9600);
  WiFi.hostname("WorkToken-Server");  // Optional: makes it easier to find on LAN
  connectToWiFi();
  LittleFS.begin();  // Mount filesystem
  setupServer();
  Serial.println("Server started!");
}

void loop(void) {
  handleClientRequests();
}
