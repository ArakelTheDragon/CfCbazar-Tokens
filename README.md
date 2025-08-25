# 🪙 Tokens Repository

This is the official repository for our token ecosystem, featuring:

- **WorkTokenStable** on Binance Smart Chain (ERC-20)
- **WorkTH** on MintMe Blockchain (MINTME)

---
Its now possible to **buy & sell** WorkTokens on https://cc.free.bg/workth/


  
Got it! Since this README is meant to serve as the GitHub front page for the CfCbazar + WorkToken system, we can make it more general, welcoming, and overview-focused rather than just an API reference. It should explain the ecosystem, how WTK works, and link to the API and other resources. Here’s a polished version ready for README.md:

# CfCbazar — WorkToken (WTK) Ecosystem

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Welcome to **CfCbazar**, the platform where users can earn, use, and interact with **WorkToken (WTK)**, a decentralized BEP-20 token on Binance Smart Chain (BSC). This repository contains documentation, APIs, and tools for integrating CfCbazar and WorkToken into your applications, IoT devices, and web services.

---

## Overview

**WorkToken (WTK)** powers the CfCbazar ecosystem, which includes:

- **Games** — Earn tokens while playing.  
- **Marketplace** — Trade tokens and unlock platform features.  
- **Platform Credit** — Convert WTK or BNB into internal credits to access premium features.  

WTK is fully **upgradeable via UUPS proxy** on BSC, ensuring a flexible and community-driven platform.

- **Proxy Contract Address:** `0xecbD4E86EE8583c8681E2eE2644FC778848B237D`  
- **Platform Credit Address:** `0xFBd767f6454bCd07c959da2E48fD429531A1323A`  

---

## Tokenomics

- **Token Name:** WorkToken  
- **Symbol:** WTK  
- **Decimals:** 18  
- **Total Supply:** Dynamic (minted/burned on buy/sell)  
- **Buy/Sell:** Market-driven minting and burning  
- **Reserve:** BNB backing liquidity  
- **Recycling:** Unsold tokens may be burned to manage supply

Platform Credit (internal currency) can **unlock features and games** but is **non-withdrawable**.

---

## API & Integration

ESP8266 devices and web apps can interact with CfCbazar using the **WorkToken API**:

- **Base URL:** `http://cfcbazar.atwebpages.com/api.php`  
- **Methods:** GET (check tokens), POST (increment tokens)  
- **Rate-limiting:** 1 increment every 5 seconds  
- **Tampering prevention:** Only 0.00001 per increment; wrong values reset tokens

**ESP Example Code:**

```cpp
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
      Serial.println(http.getString());
    }
    http.end();
  }
}

Full API documentation can be found in API.md.


---

User Flow

1. Connect wallet (MetaMask) to BSC network.


2. Buy or receive WTK tokens.


3. Deposit WTK or BNB to platform address.


4. Convert to Platform Credit.


5. Use credits for games, marketplace access, or premium features.




---

Security & Governance

Built with OpenZeppelin libraries

UUPS Proxy upgradeable smart contract

Restricted and secured owner functions

Events emitted for transparency

Governance by CfCbazar team; DAO-style governance may come later



---

Resources

Main Platform: https://cfcbazar.ct.ws/

Free Server: https://cc.free.bg/

Buy WTK: https://cc.free.bg/workth/

GitHub Repository: https://github.com/ArakelTheDragon/Tokens

Contact: cfcbazar@gmail.com



---

Disclaimer

WorkToken and Platform Credit are experimental utility tokens for access and participation in the CfCbazar ecosystem. They are not financial investments.
