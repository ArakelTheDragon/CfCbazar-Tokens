// Replace only the email with your registerred email on cfcbazar.ct.ws
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <lwip/napt.h>
#include <lwip/dns.h>

#define NAPT 1000
#define NAPT_PORT 10

const char* extenderSSID = "cfcbazar.ct.ws";
const char* extenderPassword = "12345678";
const char* apiURL = "http://cfc-api.atwebpages.com/api.php";
const char* userEmail = "cfcbazar@gmail.com";  // Replace with actual email registerred on cfcbazar.ct.ws

unsigned long lastApiCall = 0;
const unsigned long apiInterval = 5000; // 5 seconds

void sendTokenData(const String& email, float tokens) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, apiURL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "email=" + email + "&tokens=" + String(tokens, 5);
    int httpCode = http.POST(postData);

    if (httpCode == 200) {
      String response = http.getString();
      Serial.printf("✅ POST sent: %s\n", postData.c_str());
      Serial.printf("📨 Response: %s\n", response.c_str());
    } else if (httpCode == 429) {
      Serial.println("⏱️ Rate limit: wait 5 seconds");
    } else if (httpCode == 400) {
      Serial.println("❌ Invalid email or token value");
    } else {
      Serial.printf("❌ HTTP error %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("❌ WiFi not connected");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n🔧 Starting ESP8266 Range Extender with Captive Portal");

  // Start captive portal for Wi-Fi config
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.autoConnect("ESP_Config");

  Serial.println("✅ Connected to main WiFi:");
  Serial.println(WiFi.localIP());

  // Set up NATed AP
  IPAddress apIP(172, 217, 28, 254);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(extenderSSID, extenderPassword);
  Serial.printf("📡 AP '%s' started at %s\n", extenderSSID, WiFi.softAPIP().toString().c_str());

  auto& server = WiFi.softAPDhcpServer();
  server.setDns(WiFi.dnsIP(0));

  err_t ret = ip_napt_init(NAPT, NAPT_PORT);
  if (ret == ERR_OK) {
    ret = ip_napt_enable_no(SOFTAP_IF, 1);
    if (ret == ERR_OK) {
      Serial.println("🔁 NAPT enabled. Devices connected to AP will be NATed.");
    } else {
      Serial.println("❌ Failed to enable NAPT.");
    }
  } else {
    Serial.println("❌ NAPT initialization failed.");
  }

  Serial.println("✅ Setup complete.");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastApiCall >= apiInterval) {
    lastApiCall = currentMillis;

    // Always send 0.00001 to comply with API rules
    sendTokenData(userEmail, 0.00001);
  }
}
