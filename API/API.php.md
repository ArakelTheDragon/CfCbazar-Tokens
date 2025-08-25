CfCbazar WorkToken API

This API allows ESP8266 devices and the CfCbazar web platform to track and update WorkTokens for users. It includes rate-limiting, token tampering prevention, and remote synchronization between servers.


---

Base URL

http://cfcbazar.atwebpages.com/api.php


---

Supported Methods

1. GET — Retrieve current tokens

Fetch the current tokens_earned for a given email.

Request

GET /api.php?email=<user_email>

Example

curl http://cfcbazar.atwebpages.com/api.php?email=cfcbazar@gmail.com

Response

{
  "success": true,
  "email": "cfcbazar@gmail.com",
  "tokens_earned": 0.00005
}

Errors

400 Bad Request – Invalid or missing email

404 Not Found – Email not registered



---

2. POST — Update tokens

Update the tokens_earned for a user.

ESP devices must always send 0.00001 per request every 5 seconds.

Sending any other value resets the tokens to 0.


Request Parameters (POST)

Parameter	Type	Description

email	string	User email (must exist in workers table)
tokens	float	Tokens to add (ESP must send 0.00001, reset 0)


Examples

Normal increment (ESP)

curl -X POST http://cfcbazar.atwebpages.com/api.php \
     -d "email=cfcbazar@gmail.com" \
     -d "tokens=0.00001"

Response

{
  "success": true,
  "email": "cfcbazar@gmail.com",
  "tokens_delta": 0.00001,
  "timestamp": "2025-08-25 17:40:33"
}

Reset tokens (e.g., from test API or wrong increment)

curl -X POST http://cfcbazar.atwebpages.com/api.php \
     -d "email=cfcbazar@gmail.com" \
     -d "tokens=0"

Response

{
  "success": true,
  "action": "reset",
  "reason": "Invalid token value",
  "email": "cfcbazar@gmail.com"
}


---

Rate Limiting

A user can only submit a new 0.00001 increment every 5 seconds.

Submitting faster will return:


{
  "success": false,
  "error": "Rate limit: wait 5 seconds"
}

Uses the last_mine_time field in the workers table to enforce timing.



---

Local Test and Sync

The CfCbazar.ct.ws/testapi.php script:

1. Reads tokens_earned from CfCbazar.atwebpages.com.


2. Adds them to its own database.


3. Resets tokens_earned on CfCbazar.atwebpages.com to 0.



This ensures ESP increments are not double-counted.


---

Errors

HTTP Code	Reason	Description

400	Bad Request	Invalid email or token value
404	Not Found	User not registered
429	Rate Limit	ESP tried to send tokens too soon
405	Method Not Allowed	Only GET and POST are supported



---

Notes for ESP Implementation

Connect to http://cfcbazar.atwebpages.com/api.php with POST.

Send 0.00001 tokens every 5 seconds.

If any other value is sent, tokens will be reset.

Check tokens_earned with GET for display or verification.



---

Example ESP Request

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

void sendToken(const char* email) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://cfcbazar.atwebpages.com/api.php");
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
