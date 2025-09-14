# ⚙️ CfCbazar WorkToken API

The **CfCbazar WorkToken API** enables ESP8266 devices and the CfCbazar web platform to track and update user WorkTokens in real time. It includes built-in rate limiting, tamper prevention, and remote synchronization between distributed servers.

---

## 🌐 Base URL

```
http://cfc-api.atwebpages.com/api.php
```

---

## 📡 Supported Methods

### 1️⃣ `GET` — Retrieve Current Tokens

Fetch the current `tokens_earned` for a registered user.

#### 🔸 Request Format
```
GET /api.php?email=<user_email>
```

#### 🔸 Example
```bash
curl http://cfc-api.atwebpages.com/api.php?email=cfcbazar@gmail.com
```
#### Email source
```bash
email is taken from your registration on cfcbazar.ct.ws with that email.
```

#### 🔸 Successful Response
```json
{
  "success": true,
  "email": "cfcbazar@gmail.com",
  "tokens_earned": 0.00005
}
```

#### 🔸 Error Codes
| Code | Description                |
|------|----------------------------|
| 400  | Invalid or missing email   |
| 404  | Email not registered       |

---

### 2️⃣ `POST` — Update Tokens

Update the `tokens_earned` for a user.  
ESP8266 devices must send exactly `0.00001` tokens every 5 seconds.  
Any other value will reset the user's token balance to `0`.

#### 🔸 Request Parameters

| Parameter | Type   | Description                              |
|-----------|--------|------------------------------------------|
| `email`   | string | User email (must exist in `workers` table) |
| `tokens`  | float  | Tokens to add (must be `0.00001`)         |

#### 🔸 Normal Increment (ESP)
```bash
curl -X POST http://cfc-api.atwebpages.com/api.php \
     -d "email=cfcbazar@gmail.com" \
     -d "tokens=0.00001"
```

##### Response
```json
{
  "success": true,
  "email": "cfcbazar@gmail.com",
  "tokens_delta": 0.00001,
  "timestamp": "2025-08-25 17:40:33"
}
```

#### 🔸 Reset Tokens (Invalid Value)
```bash
curl -X POST http://cfc-api.atwebpages.com/api.php \
     -d "email=cfcbazar@gmail.com" \
     -d "tokens=0"
```

##### Response
```json
{
  "success": true,
  "action": "reset",
  "reason": "Invalid token value",
  "email": "cfcbazar@gmail.com"
}
```

---

## ⏱️ Rate Limiting

Users can only submit a new `0.00001` increment every **5 seconds**.  
Requests made too quickly will return:

```json
{
  "success": false,
  "error": "Rate limit: wait 5 seconds"
}
```

Rate limiting is enforced using the `last_mine_time` field in the `workers` table.

---

## 🔄 Local Test & Sync

The script at `cfc-api.ct.ws/testapi.php` performs:

1. Reads `tokens_earned` from `CfCbazar.atwebpages.com`
2. Adds them to its own local database
3. Resets `tokens_earned` remotely to prevent double-counting

---

## ⚠️ Error Reference

| HTTP Code | Reason           | Description                          |
|-----------|------------------|--------------------------------------|
| 400       | Bad Request      | Invalid email or token value         |
| 404       | Not Found        | User not registered                  |
| 429       | Rate Limit       | ESP tried to send tokens too soon    |
| 405       | Method Not Allowed | Only `GET` and `POST` are supported |

---

## 📲 ESP8266 Integration

ESP devices should connect via `POST` every 5 seconds with `0.00001` tokens.  
Any deviation will reset the user's token balance.

### 🔧 Example ESP Code WiFi Extender replace only email with your registerred email on cfcbazar.ct.ws
```cpp
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
const char* userEmail = "cfcbazar@gmail.com";  // Replace with actual email

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
```
