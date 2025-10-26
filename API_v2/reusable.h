// reusable.h — CfCbazar NAPT Extender + API + Mining Logic
#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <lwip/napt.h>
#include <lwip/dns.h>

// Main network credentials (STA)
#ifndef STASSID
#define STASSID "TP-Link_CCC78"
#define STAPSK  "69538001"
#endif

// Extender network credentials (AP)
const char* AP_SSID = "https://cfcbazar.42web.io";
const char* AP_PASS = "WorkToken";

// CfCbazar API endpoint
const char* cfcbazarApiUrl = "http://cfc-api.atwebpages.com/api.php?email=cfcbazar.payments@gmail.com";

// NAPT config
#define NAPT       1000
#define NAPT_PORT  10

// Miner identity
const char* minerEmail = "cfcbazar.payments@gmail.com";
const char* minerMAC   = "C8:2B:96:23:11:1D";  // Replace with actual MAC if needed

unsigned long lastMineTime = 0;
const unsigned long mineInterval = 5000;  // 5 seconds

/**
 * Setup the ESP8266 as a NAPT-based Wi-Fi extender.
 */
void setupExtender() {
  Serial.begin(9600);
  Serial.println("\n\nNAPT Range Extender Starting...");
  Serial.printf("Heap on start: %d\n", ESP.getFreeHeap());

  // Connect to upstream Wi-Fi (STA)
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

  Serial.printf("\nSTA connected: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("DNS: %s / %s\n", WiFi.dnsIP(0).toString().c_str(), WiFi.dnsIP(1).toString().c_str());

  // Set DNS for DHCP clients on AP
  auto& dhcp = WiFi.softAPDhcpServer();
  dhcp.setDns(WiFi.dnsIP(0));

  // Configure AP IP and subnet
  WiFi.softAPConfig(
    IPAddress(172, 217, 28, 254),
    IPAddress(172, 217, 28, 254),
    IPAddress(255, 255, 255, 0)
  );

  // Start AP with custom name and password
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP started: %s\n", WiFi.softAPIP().toString().c_str());

  // Enable NAPT
  Serial.printf("Heap before NAPT: %d\n", ESP.getFreeHeap());
  err_t ret = ip_napt_init(NAPT, NAPT_PORT);
  Serial.printf("ip_napt_init: ret=%d (OK=%d)\n", ret, ERR_OK);

  if (ret == ERR_OK) {
    ret = ip_napt_enable_no(SOFTAP_IF, 1);
    Serial.printf("ip_napt_enable_no: ret=%d (OK=%d)\n", ret, ERR_OK);
    if (ret == ERR_OK) {
      Serial.printf("Extender '%s' is now NATed behind '%s'\n", AP_SSID, STASSID);
    }
  }

  Serial.printf("Heap after NAPT: %d\n", ESP.getFreeHeap());
  if (ret != ERR_OK) {
    Serial.println("NAPT initialization failed.");
  }
}

/**
 * Reconnect STA if disconnected.
 */
void maintainSTAConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("STA disconnected, attempting reconnect...");
    WiFi.disconnect();
    WiFi.begin(STASSID, STAPSK);
  }
}

/**
 * Fetch token status from CfCbazar API.
 * Prints email and tokens_earned to Serial.
 */
void fetchCfCbazarStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Skipping CfCbazar API call.");
    return;
  }

  WiFiClient client;
  HTTPClient http;
  http.begin(client, cfcbazarApiUrl);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("CfCbazar API Response:");
    Serial.println(payload);

    int emailStart = payload.indexOf("\"email\":\"");
    int tokenStart = payload.indexOf("\"tokens_earned\":");

    if (emailStart != -1 && tokenStart != -1) {
      String email = payload.substring(emailStart + 9, payload.indexOf("\"", emailStart + 9));
      String tokenStr = payload.substring(tokenStart + 16);
      tokenStr.replace("}", "");
      tokenStr.trim();
      double tokens = tokenStr.toDouble();

      Serial.printf("Email: %s\n", email.c_str());
      Serial.printf("Tokens Earned: %.6f\n", tokens);
    } else {
      Serial.println("Failed to parse CfCbazar JSON.");
    }
  } else {
    Serial.printf("CfCbazar API GET failed, code: %d\n", httpCode);
  }

  http.end();
}

/**
 * Mine WorkTHR by sending a valid POST every 5 seconds.
 */
void mineWorkTHR() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastMineTime < mineInterval) return;

  WiFiClient client;
  HTTPClient http;
  http.begin(client, "http://cfc-api.atwebpages.com/api.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String body = "email=" + String(minerEmail) +
                "&tokens=0.00001" +
                "&mac_address=" + String(minerMAC) +
                "&token_type=WorkTHR";

  int httpCode = http.POST(body);
  String response = http.getString();

  Serial.printf("Mining POST %d: %s\n", httpCode, response.c_str());
  http.end();

  lastMineTime = millis();
}
