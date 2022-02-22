#include <Arduino.h>

// Librairie et adresse OLED version 1
//************************************
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#define I2C_ADDRESS 0x3C  // 0x3C ou 0x78 
//#define I2C_ADDRESS 0x78  // 0x3C ou 0x78 
#define RST_PIN -1
SSD1306AsciiWire oled;    // 


void setup()
{

// Initialisation de l'afficheur I2C, OLED
// ----------------------------------------------
  Wire.begin();
  Wire.setClock(400000L);     // vitesse transfert i2c , 400000L ou 400.000 Hz
  
// Init OLED version 1
//********************
  #if RST_PIN >= 0                
    oled.begin(&SH1106_128x64, I2C_ADDRESS, RST_PIN);
  #else // RST_PIN >= 0
    oled.begin(&SH1106_128x64, I2C_ADDRESS);
  #endif // RST_PIN >= 0


  oled.setFont(Adafruit5x7);

  uint32_t m = micros();
  oled.clear();
  oled.set2X();
  oled.println(" Wattmetre");
 // oled.set1X();
  oled.println("    2400");
  oled.set1X();
  oled.setCursor(0,5);
  oled.println("       Teensy 4.0");
  oled.setCursor(0,6);
  oled.println("         by F6BUA");
   
  delay(4000);              // délai maintien de l'écran de démarrage pendant 4s

 // lcd.clear();
 // oled.clear();

}


void loop()
{
 
}