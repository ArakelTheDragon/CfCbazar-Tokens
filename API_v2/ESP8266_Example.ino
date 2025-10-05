#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid     = "WIFIName";
const char* password = "WiFiPassword";

const char* apiUrl   = "http://cfc-api.atwebpages.com/api.php"; // API URL
const char* email    = "cfcbazar.payments@gmail.com"; // your email
String mac = WiFi.macAddress();

WiFiClient client;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String postData = "email=" + String(email) + "&mac_address=" + String(mac) + "&tokens=0.00001";

    http.begin(client, apiUrl); // ✅ Updated syntax
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpCode = http.POST(postData);
    String response = http.getString();

    Serial.println("POST sent:");
    Serial.println(postData);
    Serial.println("Response:");
    Serial.println(response);

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }

  delay(5000); // Wait 5 seconds before next mine
}
