Here’s a polished and GitHub-ready description of your CfCbazar API and ESP8266 Wi-Fi extender integration — perfect for your README or documentation. It explains the concept, API behavior, and includes working ESP8266 examples for both mining and extender setup.

---

# 🌐 CfCbazar Public Hotspot System

**CfCbazar** is a decentralized, token-incentivized public Wi-Fi sharing system. It rewards users with **WorkTokens (WTK)** and **WorkTHR** for donating bandwidth (≥3 Mbps) via ESP8266-based Wi-Fi extenders. These extenders create public access points while routing traffic through the donor’s internet using NAT (NAPT).

---

## 🎯 Project Goals

- Build a **usable public hotspot mesh** powered by community-donated Wi-Fi.
- Reward contributors with **crypto tokens** for uptime and bandwidth.
- Use **low-cost ESP8266 boards** (e.g., Wemos D1 Mini, ESP01) as plug-and-play extenders.
- Track and reward devices via MAC address and cooldown-enforced mining.

---

## 🔐 CfCbazar API Overview

### 🔹 `GET /api.php?email=...`

Returns current token balances and device mining history for a user.

**Example Response:**
```json
{
  "success": true,
  "email": "cfcbazar.payments@gmail.com",
  "tokens_earned": 0.00001,
  "mintme": 0.00000,
  "devices": [
    {
      "mac_address": "C8:2B:96:23:11:1D",
      "last_mine_time": "2025-10-11 11:20:58"
    }
  ]
}
```

---

### 🔹 `POST /api.php`

Used to mine tokens or reset balances.

| Field         | Type   | Description                                 |
|---------------|--------|---------------------------------------------|
| `email`       | string | User email (required)                       |
| `tokens`      | float  | Must be exactly `0.00001` to mine           |
| `mac_address` | string | Device MAC address (required)               |
| `token_type`  | string | `WorkToken` or `WorkTHR` (default: `WorkToken`) |

**Mining Example (valid):**
```x-www-form-urlencoded
email=cfcbazar.payments@gmail.com
tokens=0.00001
mac_address=C8:2B:96:23:11:1D
token_type=WorkTHR
```

**Cooldown:** 5 seconds per MAC address  
**Invalid token values** will reset the user’s balance to `0`.

---

## 📡 ESP8266 Wi-Fi Extender + Miner

This firmware turns an ESP8266 into a dual-mode device:

- Connects to your home Wi-Fi (STA)
- Creates a public AP (e.g., `CfCbazar_Extender`)
- NATs traffic from AP to STA
- Mines WorkTHR every 5 seconds via the CfCbazar API

---

### 🔧 Hardware Requirements

- ESP8266 board (Wemos D1 Mini, NodeMCU, ESP01 + breakout)
- 5V USB power supply
- Optional: OLED display or LED for status

---

### 🧠 Firmware Features

- NAPT-based Wi-Fi extender using `lwip/napt.h`
- Auto-reconnect to upstream Wi-Fi
- Periodic mining POSTs with cooldown
- Token status polling every 60 seconds
- Modular `reusable.h` for easy integration

---

### 🧪 Example: `main.ino`

```cpp
#include "reusable.h"

unsigned long lastApiCheck = 0;
const unsigned long apiCheckInterval = 60000;

void setup() {
  setupExtender();
  Serial.println("setupExtender() ok");
}

void loop() {
  maintainSTAConnection();

  if (millis() - lastApiCheck > apiCheckInterval) {
    fetchCfCbazarStatus();
    lastApiCheck = millis();
  }

  mineWorkTHR();  // Mines every 5s if cooldown met
  delay(1000);
}
```

---

### 🧩 Example: `reusable.h` Highlights

```cpp
const char* minerEmail = "cfcbazar.payments@gmail.com";
const char* minerMAC   = "C8:2B:96:23:11:1D";

void mineWorkTHR() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastMineTime < 5000) return;

  WiFiClient client;
  HTTPClient http;
  http.begin(client, "http://cfc-api.atwebpages.com/api.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String body = "email=" + String(minerEmail) +
                "&tokens=0.00001" +
                "&mac_address=" + String(minerMAC) +
                "&token_type=WorkTHR";

  http.POST(body);
  http.end();
  lastMineTime = millis();
}
```

---

## 🌍 Join the Mesh

- Plug in an ESP8266 extender
- Share at least 3 Mbps of your Wi-Fi
- Earn WorkTokens for every 5 seconds of uptime
- Help build a decentralized, token-powered public internet

---

## 📦 Repository Structure

```
/src
  ├── main.ino
  ├── reusable.h
/docs
  └── api.md
```

---

Let me know if you’d like a badge, QR code generator, or onboarding flow for new contributors. You’re building a real-world crypto mesh — and this README is your gateway to scale.
