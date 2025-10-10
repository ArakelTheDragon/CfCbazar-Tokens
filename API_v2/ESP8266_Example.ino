#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid     = "WiFiName";
const char* password = "WiFiPassword";

const char* apiUrl   = "http://cfc-api.atwebpages.com/api.php"; // API endpoint, don't change
const char* email    = "cfcbazar.payments@gmail.com"; // Replace with your mail, register on cfcbazar.ct.ws/d.php
String mac = WiFi.macAddress();           // don't change, used for validation with the server

WiFiClient client;
unsigned long lastMineTime = 0;
bool mineWorkToken = true;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long currentTime = millis();
    if (currentTime - lastMineTime >= 5000) {
      String tokenType = mineWorkToken ? "WorkToken" : "WorkTHR";
      mineToken(tokenType);
      mineWorkToken = !mineWorkToken;
      lastMineTime = currentTime;
    }
  } else {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.begin(ssid, password);
    delay(1000);
  }
}

void mineToken(String tokenType) {
  HTTPClient http;
  http.begin(client, apiUrl);  // ✅ FIXED: pass WiFiClient explicitly
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "email=" + String(email) +
                    "&mac_address=" + String(mac) +
                    "&tokens=0.00001" +
                    "&token_type=" + tokenType;

  int httpCode = http.POST(postData);
  String response = http.getString();

  Serial.println("[" + tokenType + "] HTTP Code: " + String(httpCode));
  Serial.println("Response: " + response);
  http.end();
}
