//
// 128X64 I2C ST7567S COG Graphic Display ARDUINO
//
// https://www.youtube.com/c/LeventeDaradici/videos
// the display was bought from here: https://www.aliexpress.com/item/1005004617618178.html
// short review: https://satelit-info.com/phpBB3/viewtopic.php?f=172&t=3338
//
#include <U8g2lib.h>
#include <Wire.h>

// SDA D4, SCL D3 
U8G2_ST7567_ENH_DG128064I_F_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);
 
void setup(void)
     {
        u8g2.setI2CAddress(0x3F * 2);
        u8g2.begin();
        u8g2.clearBuffer(); // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr);
        //u8g2.setFont(u8g2_font_cu12_tr);
        u8g2.drawStr(0, 10, "Hello World!"); // write something to the internal memory
        u8g2.sendBuffer(); // transfer internal memory to the display
      }
void loop(void)
     {
 
     }
