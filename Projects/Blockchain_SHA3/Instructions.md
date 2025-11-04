This is a full blockchain with nodeMCU ESP8266 as a server and client. The original BTC chain started with low difficulty as well and this is a similar system with hashing up to 2 leading zeroes.

It uses LittleFS for storing blocks. Active blocks can be pushed to littleFS from backend and pushed back to backend from littleFS. They stay on the MCU only to handle mining operaions for active miners.

This is in alpha test. It's not implemented with the Worktoken yet, but it might be in future updates.
