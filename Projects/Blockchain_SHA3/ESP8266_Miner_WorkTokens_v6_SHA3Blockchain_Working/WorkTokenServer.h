#ifndef WORKTOKEN_SERVER_H
#define WORKTOKEN_SERVER_H

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <SHA3.h>
#include <vector>
#include <ArduinoJson.h>
#include <LittleFS.h>


// Wi-Fi credentials
const char* ssid = "TP-Link_CCC7";
const char* password = "69538001";


// Web server instance
//extern ESP8266WebServer server;
ESP8266WebServer server(80);

// Job parameters
const char* jobMessage = "Genesis block for WorkToken alpha test";
const char* jobTimestamp = "2025-10-30T23:15:00Z";
const char* jobDifficulty = "00";  // 1 hex byte = 2 leading zero bytes

// Global hash buffer
byte globalHash[32];

// Miner tracking
std::vector<String> minerMACs;

bool isMACKnown(const String& mac) {
  for (size_t i = 0; i < minerMACs.size(); i++) {
    if (minerMACs[i] == mac) return true;
  }
  return false;
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  /*
  if (!WiFi.config(local_IP, gateway, subnet, dns)) {
    Serial.println("❌ Failed to configure static IP");
  } else {
    Serial.print("Local IP: ");
    Serial.println(local_IP);
    Serial.print("Subnet: ");
    Serial.println(subnet);
  }
  */
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void printHex(const byte *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 16) Serial.print("0");
    Serial.print(data[i], HEX);
  }
  Serial.println();
}

String toHex(const byte *data, size_t len) {
  String hex = "";
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) hex += "0";
    hex += String(data[i], HEX);
  }
  return hex;
}

void hashMessage(const char *message) {
  SHA3_256 sha3;
  sha3.reset();
  sha3.update((const byte *)message, strlen(message));
  sha3.finalize(globalHash, sizeof(globalHash));
}

void setupServer() {
  // Serve the next unmined block
  server.on("/getjob", HTTP_GET, []() {
    Serial.println("GET /getjob");

    int blockNum = 1;
    String blockPath;
    DynamicJsonDocument doc(512);
    bool found = false;

    while (true) {
      blockPath = "/blocks/block_" + String(blockNum) + ".json";
      if (!LittleFS.exists(blockPath)) break;

      File f = LittleFS.open(blockPath, "r");
      DeserializationError err = deserializeJson(doc, f);
      f.close();
      if (err) {
        Serial.println("❌ Failed to parse block " + String(blockNum));
        blockNum++;
        continue;
      }

      if (!doc["mined"]) {
        found = true;
        break;
      }

      blockNum++;
    }

    if (!found) {
      server.send(404, "application/json", "{\"error\":\"No unmined blocks available\"}");
      return;
    }

    String payload;
    serializeJson(doc, payload);
    server.send(200, "application/json", payload);
  });

  // Accept a mined block and mark it as mined
  server.on("/submit", HTTP_POST, []() {
    Serial.println("POST /submit");
    String message = server.arg("message");
    String nonce = server.arg("nonce");
    String mac = server.arg("mac");

    if (!mac.isEmpty() && !isMACKnown(mac)) {
      minerMACs.push_back(mac);
      Serial.println("New miner detected: " + mac);
    }

    String input = message + nonce;
    hashMessage(input.c_str());

    if (globalHash[0] == 0x00 && globalHash[1] == 0x00) {
      String hashHex = toHex(globalHash, 32);
      Serial.println("✅ Valid block mined!");
      Serial.println("Nonce: " + nonce);
      Serial.print("Hash: ");
      printHex(globalHash, 32);

      // Mark block as mined
      int blockNum = 1;
      while (true) {
        String path = "/blocks/block_" + String(blockNum) + ".json";
        if (!LittleFS.exists(path)) break;

        File f = LittleFS.open(path, "r");
        DynamicJsonDocument doc(512);
        deserializeJson(doc, f);
        f.close();

        if (doc["message"] == message && !doc["mined"]) {
          doc["mined"] = true;
          File out = LittleFS.open(path, "w");
          serializeJson(doc, out);
          out.close();
          break;
        }

        blockNum++;
      }

      server.send(200, "application/json", "{\"status\":\"Block accepted\",\"hash\":\"" + hashHex + "\"}");
    } else {
      server.send(400, "application/json", "{\"error\":\"Invalid hash\"}");
    }
  });

  // Dashboard at root
  server.on("/", HTTP_GET, []() {
    Serial.println("GET /");
    String html = "<html><head><title>WorkToken Server Dashboard</title></head><body>";
    html += "<h2>Connected Miners</h2>";
    html += "<p><b>Total:</b> " + String(minerMACs.size()) + "</p>";
    html += "<ul>";
    for (size_t i = 0; i < minerMACs.size(); i++) {
      html += "<li>" + minerMACs[i] + "</li>";
    }
    html += "</ul></body></html>";
    server.send(200, "text/html", html);
  });

  server.begin();
  Serial.println("Web server started");
}

void handleClientRequests() {
  server.handleClient();
}

#endif
