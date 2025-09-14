CfCbazar WorkToken API

This API allows ESP8266 devices and the CfCbazar web platform to track and update WorkTokens for users. It includes rate-limiting, token tampering prevention, and remote synchronization between servers.


---

Base URL

http://cfc-api.atwebpages.com/api.php


---

Supported Methods

1. GET — Retrieve current tokens

Fetch the current tokens_earned for a given email.

Request

GET /api.php?email=<user_email>

Example

curl http://cfc-api.atwebpages.com/api.php?email=cfcbazar@gmail.com

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

curl -X POST http://cfc-api.atwebpages.com/api.php \
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

curl -X POST http://cfc-api.atwebpages.com/api.php \
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

The cfc-api.ct.ws/testapi.php script:

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

Connect to http://cfc-api.atwebpages.com/api.php with POST.

Send 0.00001 tokens every 5 seconds.

If any other value is sent, tokens will be reset.

Check tokens_earned with GET for display or verification.

---
Example api.php
<?php
// ----- API.php (CfCbazar.atwebpages.com) -----
require 'config.php';
header('Content-Type: application/json');

// Validate email format
function is_valid_email(string $email): bool {
    return filter_var($email, FILTER_VALIDATE_EMAIL) !== false;
}

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'POST') {
    $email  = $_POST['email'] ?? '';
    $tokens = $_POST['tokens'] ?? null;

    if (!is_valid_email($email) || !is_numeric($tokens)) {
        http_response_code(400);
        echo json_encode(['success' => false, 'error' => 'Invalid email or token value']);
        exit;
    }

    $tokens = floatval($tokens);

    // If tokens != 0.00001, reset tokens_earned
    if ($tokens !== 0.00001) {
        $stmt = $conn->prepare("UPDATE workers SET tokens_earned = 0 WHERE email = ?");
        $stmt->bind_param("s", $email);
        $ok = $stmt->execute();
        $stmt->close();

        echo json_encode([
            'success' => $ok,
            'action'  => 'reset',
            'reason'  => 'Invalid token value',
            'email'   => $email
        ]);
        exit;
    }

    // Enforce 5 sec rule using last_mine_time
    $stmt = $conn->prepare("SELECT last_mine_time FROM workers WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $stmt->bind_result($last_mine_time);
    $stmt->fetch();
    $stmt->close();

    $now = time();
    if ($last_mine_time && ($now - strtotime($last_mine_time)) < 5) {
        http_response_code(429);
        echo json_encode(['success' => false, 'error' => 'Rate limit: wait 5 seconds']);
        exit;
    }

    // Update tokens and last_mine_time
    $stmt = $conn->prepare("
        UPDATE workers
           SET tokens_earned = tokens_earned + ?,
               last_mine_time = NOW()
         WHERE email = ?
    ");
    $stmt->bind_param("ds", $tokens, $email);
    $ok = $stmt->execute();
    $stmt->close();

    echo json_encode([
        'success'      => $ok,
        'email'        => $email,
        'tokens_delta' => $tokens,
        'timestamp'    => date('Y-m-d H:i:s')
    ]);
    exit;
}

if ($method === 'GET') {
    $email = $_GET['email'] ?? '';
    if (!is_valid_email($email)) {
        http_response_code(400);
        echo json_encode(['success' => false, 'error' => 'Invalid email']);
        exit;
    }

    $stmt = $conn->prepare("SELECT tokens_earned FROM workers WHERE email = ?");
    $stmt->bind_param("s", $email);
    $stmt->execute();
    $stmt->bind_result($tokens);
    if ($stmt->fetch()) {
        echo json_encode([
            'success'       => true,
            'email'         => $email,
            'tokens_earned' => (float)$tokens
        ]);
    } else {
        http_response_code(404);
        echo json_encode(['success' => false, 'error' => 'Worker not found']);
    }
    $stmt->close();
    exit;
}

http_response_code(405);
echo json_encode(['success' => false, 'error' => 'Method not allowed']);
---

Example ESP Request

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
