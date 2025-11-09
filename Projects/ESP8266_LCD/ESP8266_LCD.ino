#include <Wire.h>
#include <EEPROM.h>

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Prototypes
void vInitializeLCD();

void setup(){
  // Initialization
  vInitializeLCD();
}

void loop(){
;
}

void vInitializeLCD(){
  Wire.begin(2,0); // Start wire, needed for LCDs, D4 SDA, D3 SCL
  delay(1);
  Serial.begin(9600); // Start serial
  delay(1);

  // Init and set LCD
  lcd.init();   // initializing the LCD
  lcd.backlight(); // Enable or Turn On the backlight 
  
  // Print on LCD
  lcd.setCursor(0, 0);  // Set cursor to column 0, row 0 (first line)  
  lcd.print(" Hello World! "); // Start Printing
  lcd.setCursor(0, 1);  // Set cursor to column 0, row 0 (first line)
  lcd.print(" ESP8266 "); // Start Printing
  delay(2000); // Text to be visible on the screen
  
  // Clear LCD
  lcd.clear(); // Clear text after wait

  
}

void vInitializeEEPROM() {
  // Initialize EEPROM size of 512 bytes
  EEPROM.begin(512);
  // Writing a value to EEPROM at position 0
  int valueToWrite = 123;
  EEPROM.write(0, valueToWrite);
  // Commit to save the value in EEPROM
  EEPROM.commit();
  Serial.println("Value written to EEPROM!");
  lcd.print("Value written to EEPROM!"); // Printing
}

void vReadDisplayEEPROM(){
  lcd.clear(); // Clear old chars on the LCD
    // Read the value from EEPROM at position 0
  int storedValue = EEPROM.read(0);

  Serial.print("After reset: ");
  Serial.println(storedValue);
  lcd.setCursor(0, 0);  // Set cursor to column 0, row 0 (first line)
  lcd.print("After reset:");
  lcd.setCursor(0, 1);  // Set cursor to column 0, row 1 (first line)  
  lcd.print(storedValue);
  delay(2000);
  lcd.clear(); 
  //lcd.print(" Hello World! "); // Start Printing
}
