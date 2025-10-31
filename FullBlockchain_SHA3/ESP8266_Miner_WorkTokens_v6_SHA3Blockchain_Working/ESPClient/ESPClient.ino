#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <SHA3.h>
#include <ArduinoJson.h>

// Wi-Fi and server config
const char* ssid = "TP-Link_CCC7";
const char* password = "69538001";
const char* serverHost = "10.87.249.26";  // IP of the WorkToken server

ESP8266WebServer clientServer(80);

// Global hash buffer
byte globalHash[32];

// Miner state
String currentJob = "";
unsigned long lastNonce = 0;
String lastHashHex = "";
String clientMAC = "";
int currentBlockNumber = 0;

// Function declarations
void connectToWiFi(void);
void setupDashboard(void);
void printHex(const byte *data, size_t len);
String toHex(const byte *data, size_t len);
void hashMessage(const char *message);
bool fetchJob(String& messageOut);
bool submitNonce(const String& message, unsigned long nonce);

void setup(void) {
  Serial.begin(9600);
  connectToWiFi();
  clientMAC = WiFi.macAddress();
  setupDashboard();
}

void loop(void) {
  clientServer.handleClient();

  String baseMessage;
  if (!fetchJob(baseMessage)) {
    delay(5000);
    return;
  }

  currentJob = baseMessage;
  lastHashHex = "";
  unsigned long nonce = 0;
  char buffer[128];

  while (true) {
    snprintf(buffer, sizeof(buffer), "%s%lu", baseMessage.c_str(), nonce);
    hashMessage(buffer);
    lastNonce = nonce;

    if (globalHash[0] == 0x00 && globalHash[1] == 0x00) {
      Serial.print("✅ Valid nonce found: ");
      Serial.println(nonce);
      lastHashHex = toHex(globalHash, 32);
      submitNonce(baseMessage, nonce);
      delay(10000);  // Pause before next job
      break;
    }

    nonce++;
    if (nonce % 10000 == 0) {
      Serial.print(".");
    }

    yield();  // Prevent watchdog reset
  }
}

// Connect to WiFi
void connectToWiFi(void) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Local dashboard
void setupDashboard(void) {
  clientServer.on("/", HTTP_GET, []() {
    String html = "<html><head><title>WorkToken Miner</title></head><body>";
    html += "<h2>ESP8266 Miner Dashboard</h2>";
    html += "<p><b>WiFi:</b> " + WiFi.SSID() + "</p>";
    html += "<p><b>IP:</b> " + WiFi.localIP().toString() + "</p>";
    html += "<p><b>MAC:</b> " + clientMAC + "</p>";
    html += "<p><b>Block #:</b> " + String(currentBlockNumber) + "</p>";
    html += "<p><b>Job:</b> " + currentJob + "</p>";
    html += "<p><b>Last Nonce:</b> " + String(lastNonce) + "</p>";
    html += "<p><b>Last Hash:</b> " + lastHashHex + "</p>";
    html += "</body></html>";
    clientServer.send(200, "text/html", html);
  });

  clientServer.begin();
  Serial.println("Client dashboard started");
}

// Hashing logic
void hashMessage(const char *message) {
  SHA3_256 sha3;
  sha3.reset();
  sha3.update((const byte *)message, strlen(message));
  sha3.finalize(globalHash, sizeof(globalHash));
}

String toHex(const byte *data, size_t len) {
  String hex = "";
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) hex += "0";
    hex += String(data[i], HEX);
  }
  return hex;
}

void printHex(const byte *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 16) Serial.print("0");
    Serial.print(data[i], HEX);
  }
  Serial.println();
}

// Fetch job from server
bool fetchJob(String& messageOut) {
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + String(serverHost) + "/getjob";
  http.begin(client, url);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Job received:");
    Serial.println(payload);

    DynamicJsonDocument doc(512);
    deserializeJson(doc, payload);
    messageOut = doc["message"].as<String>();
    currentBlockNumber = doc["block_number"].as<int>();
    http.end();
    return true;
  } else {
    Serial.print("Failed to fetch job. HTTP code: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

// Submit mined block
bool submitNonce(const String& message, unsigned long nonce) {
  WiFiClient client;
  HTTPClient http;
  String url = "http://" + String(serverHost) + "/submit";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "message=" + message +
                    "&nonce=" + String(nonce) +
                    "&mac=" + clientMAC +
                    "&block_number=" + String(currentBlockNumber);

  int httpCode = http.POST(postData);

  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("✅ Block accepted:");
    Serial.println(response);
    http.end();
    return true;
  } else {
    Serial.println("❌ Block rejected:");
    Serial.println(http.getString());
    http.end();
    return false;
  }
}
