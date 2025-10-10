## 📦 CfCbazar Token Mining Platform

A lightweight, ESP8266-compatible token mining and management system for CfCbazar, supporting dual-token mining (WorkToken & WorkTHR), rate-limited device tracking, and platform-controlled token distribution.

### 🔧 Features

- ⛏️ **Proof-of-Work Mining API**  
  ESP8266 devices mine tokens via POST requests with cooldown enforcement and MAC-based tracking.

- 🔄 **Dual Token Support**  
  Users can mine either `WorkToken` or `WorkTHR`, with balances stored separately (`tokens_earned` and `mintme`).

- 🏦 **Platform Reserve Enforcement**  
  All mined tokens are deducted from the central platform account (`cfcbazar@gmail.com`). Mining halts if reserves are depleted.

- 📊 **Dashboard Sync**  
  `testapi.php` syncs mined tokens from the remote API to the CfCbazar dashboard and resets remote balances.

- 📡 **Device Management**  
  Tracks active/inactive mining devices with automatic deactivation after inactivity.

- 🔐 **Rate Limiting**  
  Enforces a 5-second cooldown per device to prevent abuse.

### 🧪 ESP8266 Integration

Includes example Arduino sketch to alternate mining `WorkToken` and `WorkTHR` every 5 seconds using HTTPClient.

### 📁 Structure

- `api.php` – Main mining API (remote)
- `testapi.php` – Sync script for CfCbazar dashboard
- `d.php` – User dashboard with token stats and wallet management
- `reusable2.php` – Browser-based RPC dashboard using ethers.js
- `config.php` – Database connection
- `devices` table – Tracks mining activity per MAC address
- `workers` table – Stores token balances and wallet addresses

### 🌐 Live API

- `http://cfc-api.atwebpages.com/api.php` – Remote mining endpoint
- `http://cfcbazar.ct.ws/d.php` – Dashboard interface & registration

---
