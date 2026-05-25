// Built in libs
#include <SHA3.h> // This lib comes from the Arduino IDE library manger and is a part of the Crypto lib by Dr. Brandon Wiley * bcl by Simon Downey

// Prototypes
void printHex(const byte *data, size_t len);
void hashMessage(const char *message);

// Global def, vars that won't change
#define dBAUD 9600

// Global vars, vars that change
//int i=0; // Counter
byte globalHash[32];

void setup() {
  Serial.begin(dBAUD);
  while (!Serial) {
    ; // Wait for serial module to be initialized
  }
  Serial.print("\nSerial initialized.\n");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // LED OFF initially

}

void loop() {
  static unsigned long i = 999;
  char temp[64];
  snprintf(temp, sizeof(temp), "Test WTK network! %lu", i);

  hashMessage(temp); // Updates globalHash

  // Check for 2 leading zero bytes or 4 leadng zeores
  if (globalHash[0] == 0x00 && globalHash[1] == 0x00) {
    Serial.print("✅ Match found at nonce ");
    Serial.println(i);
    delay(50000); // Pause to celebrate
    i=0;
  }

  i++;
}

// Function definitions after main()

void hashMessage(const char *message) {
  //const char *message = "Test WTK network!"; // Moved to funct
  //byte hash[32]; // SHA3-256 output // Moved to globalHash global var

  SHA3_256 sha3;
  sha3.reset();
  sha3.update((const byte *)message, strlen(message));
  sha3.finalize(globalHash, sizeof(globalHash));

  Serial.println(message);
  Serial.print("SHA3-256: ");
  printHex(globalHash, sizeof(globalHash));

  //return globalHash; // globalHash is a global var
}

void printHex(const byte *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 16) Serial.print("0");
    Serial.print(data[i], HEX);
  }
  Serial.println();
}
