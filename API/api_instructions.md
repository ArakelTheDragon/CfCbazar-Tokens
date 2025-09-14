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

### 🔧 Example ESP Code
```cpp
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

void sendToken(const char* email) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://cfc-api.atwebpages.com/api.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "email=" + String(email) + "&tokens=0.00001";
    int httpCode = http.POST(postData);

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);
    }
    http.end();
  }
}
```
