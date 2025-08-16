# 🪙 Tokens Repository

This is the official repository for our token ecosystem, featuring:

- **WorkTokenStable** on Binance Smart Chain (ERC-20)
- **WorkTH** on MintMe Blockchain (MINTME)

---
Its now possible to **buy & sell** WorkTokens on https://cc.free.bg/workth/


# WorkToken (WTK) — Whitepaper

## Overview

WorkToken (WTK) is a decentralized BEP-20 token built on the Binance Smart Chain (BSC), designed to facilitate a transparent, community-driven ecosystem with utility in gaming, marketplaces, and smart deals.  

The WorkToken smart contract is upgradeable using OpenZeppelin’s UUPS proxy pattern, ensuring flexibility and long-term adaptability.  

**Proxy Contract Address:** `0xecbD4E86EE8583c8681E2eE2644FC778848B237D`  
(Importable into MetaMask as **WTK**)

Our vision is to create a token that powers a diverse platform, including games, marketplaces, and decentralized applications, where users can buy, sell, stake, and trade seamlessly, fostering community engagement and value growth.  

We aim to unify the price of labour across regions and services by using WorkToken and platform credits as a fair medium of exchange.

---

## Tokenomics

- **Token Name:** WorkToken  
- **Symbol:** WTK  
- **Network:** Binance Smart Chain (BSC)  
- **Decimals:** 18  
- **Total Supply:** Dynamic (minted/burned on buy/sell)  
- **Buy Mechanism:** Users buy WTK by sending BNB to the contract, which mints tokens at a dynamic market price.  
- **Sell Mechanism:** Users sell WTK back to the contract, which burns tokens and sends BNB at a discounted sell price.  
- **Reserve Address:** Holds the BNB backing token liquidity.  
- **Recycling:** Unsold/recycled tokens may be burned to manage supply.  
- **Upgradeable Contract:** Implemented with UUPS proxy for future enhancements.  

---

## Platform Credit System

While WTK is a tradable BEP-20 token, the **CfCbazar platform uses non-withdrawable Platform Credit** as its internal economy.  

### Deposit Rules
- Deposit **BNB** or **WTK** to the platform address:  
  `0xFBd767f6454bCd07c959da2E48fD429531A1323A`  

- Then visit: [https://cfcbazar.ct.ws/buy.php](https://cfcbazar.ct.ws/buy.php)  
  Select the deposited coin (BNB or WTK) and press **Check Deposit**.  

- Conversion:  
  - **1 WorkToken (WTK) → 1 Platform Credit**  
  - **0.00001 BNB → 1 Platform Credit**  

- **Anti-Double Credit:** Each transaction is checked by its hash to prevent duplicate credits.  

### Usage
- **Platform Credit** can unlock premium features, access games, and participate in CfCbazar’s ecosystem.  
- It is **not withdrawable** and serves as utility inside the platform.  

---

## Smart Contract Architecture

The WorkToken smart contract includes:

- **ERC20 Standard Interface:** Token balance management, transfers, approvals.  
- **Upgradeable Proxy (UUPS):** Secure contract upgrade pattern.  
- **Dynamic Pricing:** Market price calculation based on BNB reserves and supply.  
- **Buy/Sell Functions:** Automatic minting/burning on swaps.  
- **Owner Controls:** Fund/withdraw reserves, set manual price.  
- **Burn From Recycle:** Keeps supply healthy.  

---

## User Interaction Flow

1. **Connect Wallet** (MetaMask or compatible).  
2. **Switch Network** to BSC if required.  
3. **Check Balances** of BNB and WTK.  
4. **Buy Tokens** at market price (+10%).  
5. **Sell Tokens** at market price (-10%).  
6. **Deposit to Platform** by sending WTK/BNB to platform address.  
7. **Claim Platform Credit** at [buy.php](https://cfcbazar.ct.ws/buy.php).  
8. **Use Credit** for premium features, games, and marketplace access.  

---

## Security Considerations

- Built with **OpenZeppelin libraries**.  
- **UUPS Proxy Upgradeability** with safe permissions.  
- Owner functions restricted and secured.  
- Events emitted for transparency (Buy, Sell, Burn, Withdraw, Deposit).  

---

## Roadmap

- **v1.0:** Launch on BSC mainnet with buy/sell + upgradeability.  
- **v1.1:** Deposit system + credit conversion live.  
- **v1.2:** Integration with gaming + marketplace dApps.  
- **v1.3:** Migration fully to CfCbazar main platform (cfcbazar.ct.ws).  

---

## Governance

Currently governed by the CfCbazar team. A DAO-style governance may be introduced as the ecosystem matures.  

---

## Team

- **Founder:** CfCbazar  
- **Development:** Full-stack PHP/JS + HTML5 integration  
- **Smart Contract Advisor:** Solidity with Remix IDE  

---

## Community & Contact

- **Main Platform:** [https://cfcbazar.ct.ws/](https://cfcbazar.ct.ws/)  
- **Free Server (temporary):** [https://cc.free.bg/](https://cc.free.bg/)  
- **Buy WTK:** [https://cc.free.bg/workth/](https://cc.free.bg/workth/)  
- **GitHub:** [https://github.com/ArakelTheDragon/Tokens](https://github.com/ArakelTheDragon/Tokens)  
- **Email:** [cfcbazar@gmail.com](mailto:cfcbazar@gmail.com)  

---

## Disclaimer

WorkToken and Platform Credit are **experimental utility tokens**.  
They should not be treated as financial investments but as tools to access and reward participation within the CfCbazar ecosystem.  

---
