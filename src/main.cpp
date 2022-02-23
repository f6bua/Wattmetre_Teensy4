//
// Wattmètre UHF-SHF jusque 8 GHz basé sur un logiciel de DC5ZM + AI6PK
// Adapté pour une utilisation jusque 2,4 GHz à P Max= 200W
// Avec un Teensy 4.0 et un afficheur OLED de 1,3"
// Pour des convertisseurs AD 8318 https://www.analog.com/en/products/ad8318.html
// Interface d'entrée des commandes sur le principe de la mesure en analogique sur A0
// des "LCD Keypad Shield" pour Arduino Uno
// Entrée "Select" pour choisir le menu 1 2 ou 3 (Direct, Réfléchi ou ROS)
// Entrée "Haut" et "Bas" pour choisir l'atténuation totale (ligne de mesure + l'atténuateur d'adaptation)
// Entrée "Droite" et "Gauche" pour choisir le bande de fréquence la mieux adaptée
//
// Puissance d'entrée utile sur un AD4318 < 0dBm et puissance max < 12 dBm
// Exemple: pour mesurer une puissance maxi de 100 W il faut atténuer de 50 dB
//          avec une ligne atténuant de 30dB, il faudra rajouter un bouchon de 20 dB pour avoir une mesure correcte
//          Ligne -30 dB + Bouchon -20dB   =   Atténuation totale -50dB
//          Je peux envoyer 100W soit 50dBm pour une mesure à 0dBm soit 1mW
// 
//    Ligne      Bouchon         P Mesurable       P Maximum tolérable
//    20 dB         Sans       100 mW = 20 dBm      < 32 dBm = 2 W
//    20 dB       10 dB           1 W = 30 dBm      < 42 dBm = 20 W
//    20 dB       20 dB          10 W = 40 dBm      < 52 dBm = 200 W
//    20 dB       30 dB         100 W = 50 dBm      < 200 W
//    20 dB       33 dB         200 W = 53 dBm      < 200 W          Les lignes étant limitées à 200 W, il faut s'arrêter là.
//
//    Ligne      Bouchon         P Mesurable       P Maximum tolérable
//    30 dB         Sans          1 W = 30 dBm      < 42 dBm = 20 W
//    30 dB        10 dB         10 W = 40 dBm      < 52 dBm = 200 W
//    30 dB        20 dB        100 W = 50 dBm      < 200 W
//    30 dB        23 dB        200 W = 53 dBm      < 200 W           Les lignes étant limitées à 200 W, il faut s'arrêter là.


#include <Arduino.h>
#include <EEPROM.h>   

// Librairie et adresse OLED version 1
//************************************
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#define I2C_ADDRESS 0x3C  // 0x3C ou 0x78 
#define RST_PIN -1
SSD1306AsciiWire oled;    // Simplification d'accès à l'afficheur OLED

#define LEFT 1                                            // define keys
#define RIGHT 2
#define UP 3
#define DOWN 4
#define SELECT 5
#define NONE 0

int keyPress;

//------ custom chacters made using   https://omerk.github.io/oledchargen  ???????????????????????????

byte char_up_down[8]    = {0b00100,0b01010,0b10001,0b00000,0b10001,0b01010,
                           0b00100,0b00000};

byte char_left_right[8] = {0b10000,0b01000,0b00100,0b01001,0b10010,0b00100,
                           0b00010,0b00001};
// ----------------------------------------------------------------------------------------------------- 

byte  freq_curve_nr ,  freq_curve_nr_prev, iii , key_voltage, KEY , display_menu_nr = 1;
byte att_CH1 , att_CH1_prev, att_CH2 = 0 , att_CH2_prev ;

char  float_string[7]; 
int   error_limit_LOW , error_limit_HIGH; 

float f_ghz,  mmm,  ccc ;
float voltage_CH_1, level_CH_1,  power_W_1, Return_Loss, RL_linear, SWR;
float voltage_CH_2, level_CH_2,  power_W_2 ;


// *********************************** Setup **********************************************************
// =====================================================================================================
void setup()
{
// Initialisation de l'afficheur I2C, OLED
  Wire.begin();
  Wire.setClock(400000L);     // vitesse transfert i2c , 400000L ou 400.000 Hz
  
// Init OLED
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
  oled.println("   2400");
  oled.set1X();
  oled.setCursor(0,5);
  oled.println("     Teensy 4.0");
  oled.setCursor(0,6);
  oled.println("     par  F6BUA");
   
  delay(4000);              // délai maintien de l'écran de démarrage pendant 4s

 freq_curve_nr = EEPROM.read(0);              // Récupération Courbe d'étalonnage mémorisée
    att_CH1 = EEPROM.read(1);                 // Récupération Atténuation mémorisée Canal "Direct"
    att_CH2 = EEPROM.read(2);                 // Récupération Atténuation mémorisée Canal "Réfléchi"
    
 oled.clear();
}
// ======================================= FIN du SETUP() ==================================================================================
// *****************************************************************************************************************************************


// ================================= Choix du menu Direct, réfléchi et ROS-mètre ===========================================
void select_menu()
{
  key_voltage = analogRead(A0)/10;            // div by 10 because key_voltage is type byte (0->255)

  if (key_voltage>65  && key_voltage<90 )  KEY = SELECT;    // change display_menu_nr

  if( KEY == SELECT) 
  {   if( (KEY == SELECT) && (display_menu_nr == 1) ) {display_menu_nr = 2; KEY = NONE;}
      if( (KEY == SELECT) && (display_menu_nr == 2) ) {display_menu_nr = 3; KEY = NONE;}
      if( (KEY == SELECT) && (display_menu_nr == 3) ) {display_menu_nr = 1; KEY = NONE;} 
      while(  (analogRead(A0)/10) < 100  );                       // wait for key released
  } 
       
  KEY = NONE;
}

// ============= Table des paramètres de calibration en fonction de la fréquence =======================
// -----------------------------------------------------------------------------------------------------
void AD8318_use_curve_0ghz9()
{   mmm = 0.025;                                      // values for straight line equation: mmm = slope
    ccc = 0.56;                                       // ccc = intercept y-axis
  
    error_limit_LOW = -55;                            // dBm limits for acceptable error range
    error_limit_HIGH = -1;
}

// -----------------------------------------------------------------------------------------------------
void AD8318_use_curve_1ghz9()
{   mmm = 0.025;
    ccc = 0.45;
    error_limit_LOW = -68;
    error_limit_HIGH = -5;
}

// -----------------------------------------------------------------------------------------------------
void AD8318_use_curve_2ghz4() 
{   mmm = 0.025;
    ccc = 0.45;
    error_limit_LOW = -60;
    error_limit_HIGH = -4;
}

// -----------------------------------------------------------------------------------------------------
void AD8318_use_curve_3ghz6()
{   mmm = 0.025;                                    
    ccc = 0.5;
    error_limit_LOW = -52;                      
    error_limit_HIGH = -3;
}

// -----------------------------------------------------------------------------------------------------
void AD8318_use_curve_5ghz8()
{   mmm = 0.025;
    ccc = 0.63;
    error_limit_LOW = -56;
    error_limit_HIGH = 0;
}

// -----------------------------------------------------------------------------------------------------
void AD8318_use_curve_8ghz0()
{   mmm = 0.025;
    ccc = 0.77;
    error_limit_LOW = -54;
    error_limit_HIGH =  0;
}

// ================================= Choix de la table: Direct, réfléchi et ROS-mètre ===========================================
// -----------------------------------------------------------------------------------------------------   
void select_calibration_curve()
{
  key_voltage = analogRead(A0)/10;                // div by 10 because key_voltage is byte (0->255)

  if (key_voltage <20)                      KEY = RIGHT;
  if (key_voltage>35  && key_voltage<55 )  KEY = LEFT;  
  if( KEY == RIGHT || KEY == LEFT) while( (analogRead(A0)/10) < 100  );    // wait for key released

  if(KEY == LEFT)  freq_curve_nr--;                           // freq_curve_nr = counter index, decrement
  if(KEY == RIGHT) freq_curve_nr++;                           // increment counter index
  if(freq_curve_nr < 1) freq_curve_nr = 1;                           // handle underflow of counter index
  if(freq_curve_nr > 6) freq_curve_nr = 6;                           // handle overflow ofcounter index

 // freq_curve_nr = 3;
  if(freq_curve_nr==1)  {f_ghz = 0.9; AD8318_use_curve_0ghz9();}     // select 1 of 6 calibration curves
  if(freq_curve_nr==2)  {f_ghz = 1.9; AD8318_use_curve_1ghz9();}  
  if(freq_curve_nr==3)  {f_ghz = 2.4; AD8318_use_curve_2ghz4();}  
  if(freq_curve_nr==4)  {f_ghz = 3.6; AD8318_use_curve_3ghz6();}
  if(freq_curve_nr==5)  {f_ghz = 5.8; AD8318_use_curve_5ghz8();}
  if(freq_curve_nr==6)  {f_ghz = 8.0; AD8318_use_curve_8ghz0();}

  if(freq_curve_nr_prev != freq_curve_nr)                 // if freq_curve_nr has changed ...
    { EEPROM.write(0,freq_curve_nr);                      // save selected frequency curve in EEPROM
    freq_curve_nr_prev = freq_curve_nr;
    }
  
  KEY = NONE;    
} 

// ================================== Menu Direct, Ajustement par le clavier 5 touches =========================================================== 
void select_attenuator_CH_1()
{
  key_voltage = analogRead(A0)/10;                    // check KEY
  if (key_voltage>20  && key_voltage<39 )  KEY = DOWN;
  if (key_voltage >10  && key_voltage<22 )  KEY = UP;

  

  while( (KEY == DOWN) || (KEY == UP) )
  {  
    if(KEY == DOWN)    att_CH1--;               // att = counter index, decrement
    if(KEY == UP  )    att_CH1++;               // increment att
    if(att_CH1 < 30) att_CH1 = 30; 
    if(att_CH1 ==  61) att_CH1 = 60;            // handle underflow of data type byte 
    if(att_CH1 == 255) att_CH1 = 0;             // handle overflow of attenuator      EUHHHHHHHH!
    
  
    dtostrf(att_CH1,2,0,float_string);      // format number to 2 digits, no nr. behind the dec.point
    oled.setCursor(14,2);
    oled.print(float_string);
    delay(200);

    KEY = NONE;
    key_voltage = analogRead(A0)/10;                    // check KEY again
    if (key_voltage >5  && key_voltage<15 )  KEY = UP;
    if (key_voltage>20  && key_voltage<30 )  KEY = DOWN;
  }      

  if(att_CH1_prev != att_CH1)                    // if att_CH1 has changed ...
  {EEPROM.write(1,att_CH1);                      // save selected attenuator in EEPROM
  att_CH1_prev = att_CH1;
  }
}

// ---------------------------------Lecture et Calcul de la valeur "Directe" ---------------------------------------------
void read_output_CH_1()
{ 
    voltage_CH_1  =  analogRead(A1);                           // read 1x
    for(iii=0;iii<=8; iii++)                                   // iii = counter index      
        voltage_CH_1  =  voltage_CH_1 + analogRead(A1);        // read 9x

    voltage_CH_1  =  voltage_CH_1/2046 ;                       // calculate average value of 10 readings 
}                                                              // 2046 = 10*1023/5V  

// -----------------------------------------------------------------------------------------------------
void  calculate_power_CH_1()        // used straight line equation: y = mmm*x + ccc -> x = (y - ccc)/mmm
{                                       
  level_CH_1 = -(voltage_CH_1-ccc)/mmm +att_CH1;
  level_CH_1 = floor(level_CH_1 + 0.5);                                // round and convert to integer
  power_W_1 = pow(10,level_CH_1/10);                                   // convert dBm to mW 
}

// -----------------------------------------------------------------------------------------------------
void select_subunit_of_power_CH_1()                    // select unit: W, mW, µW, nW
{ 
  if(level_CH_1 <-30)
    { dtostrf(power_W_1*1000000,4,0,float_string);     // convert float to string
      oled.print(float_string);
      oled.print(" nW ");
    }

  if(level_CH_1 >= -30 && level_CH_1 <0)
    { dtostrf(power_W_1*1000,4,0,float_string);      
      oled.print(float_string);
      oled.print(" \xE4W ");                             // \xE4W = ascii code for µW
    } 

  if(level_CH_1 >=0 && level_CH_1 <30)                        
    { dtostrf(power_W_1,4,0,float_string);    
      oled.print(float_string);
      oled.print(" mW "); 
    } 

  if(level_CH_1 >=30 && level_CH_1 < 40)
    { dtostrf(power_W_1/1000,4,1,float_string);       // 1.0W to 9.9W
      oled.print(float_string);
      oled.print(" W ");   
    }  


  if(level_CH_1 >=40 && level_CH_1 < 60)
    { dtostrf(power_W_1/1000,4,0,float_string);       // 10W to 999W
      oled.print(float_string);
      oled.print(" W ");   
    }  

  if(level_CH_1 >= 60)
    { oled.clear();
      delay(100);
      oled.print(" Power 1P > 999W  "); 
      oled.setCursor(0,1);
      oled.print(" is out of range  ");
    }
}    

// ----------------------------------   Affichage du menu "Direct"  -------------------------------------------------------------------
void display_power_CH_1()
{ 
  oled.clear();
  oled.set2X();
  oled.setCursor(0,0);                                       // go to line #1
  dtostrf(level_CH_1,3,0,float_string);                     //convert float to string
  oled.print(" ");      
  oled.print(float_string);
  oled.print(" dBm");
  
  oled.setCursor(0,3);                                       // go to line #2
  
  oled.print("Dir ");
  select_subunit_of_power_CH_1();
      
  oled.set1X();
  oled.setCursor(0,6);    
  oled.print("ATT totale = ");      
  dtostrf(att_CH1,2,0,float_string);       
  oled.print(float_string);
  oled.print("dB ");
//  oled.write(byte(0));                                       // custom made char_up_down

  oled.setCursor(0,7);
  oled.print("Bde freq = ");
  dtostrf(f_ghz,3,1,float_string);   
  oled.print(float_string);
  oled.print(" GHz ");
//  oled.write(byte(1));                                       // custom made char_left_right

  delay(500);

  if((level_CH_1 -att_CH1) < error_limit_LOW || (level_CH_1 -att_CH1) > error_limit_HIGH)
    { delay(300); oled.setCursor(0,0); oled.print(" ");  oled.setCursor(0,1); oled.print(" ");delay(300);}
}


// ===================================  Menu "Réfléchi" ========================================================== 
void select_attenuator_CH_2()
{
  key_voltage = analogRead(A0)/10;                           // check KEY
  if (key_voltage >5  && key_voltage<15 ) KEY = UP;
  if (key_voltage>20  && key_voltage<35 ) KEY = DOWN;
                                                                       
  while( (KEY == DOWN) || (KEY == UP) )
  {  
    if(KEY == DOWN)    att_CH2--;               // att = counter index, decrement
    if(KEY == UP  )    att_CH2++;               // increment att
    if(att_CH2 < 30) att_CH2 = 30; 
    if(att_CH2 ==  61) att_CH2 = 60;            // handle underflow of data type byte 
    if(att_CH2 == 255) att_CH2 = 30;             // handle overflow of attenuator
    
  
    dtostrf(att_CH2,2,0,float_string);      // format number to 2 digits, no nr. behind the dec.point
    oled.setCursor(14,2);
    oled.print(float_string);
    delay(300);

    KEY = NONE;
    key_voltage = analogRead(A0)/10;                    // check KEY again
    if (key_voltage >5  && key_voltage<15 )  KEY = UP;
    if (key_voltage>20  && key_voltage<35 )  KEY = DOWN;
  }      

  if(att_CH2_prev != att_CH2)                    // if att_CH1 has changed ...
    {EEPROM.write(2,att_CH2);                      // save selected attenuator in EEPROM
     att_CH2_prev = att_CH2;
    }
}
// -----------------------------------------------------------------------------------------------------
void read_output_CH_2()
{ 
  voltage_CH_2  =  analogRead(A2);                         // read value 1 time  
         
  for(iii=0;iii<=8; iii++)                                 // iii = counter index      
    voltage_CH_2  =  voltage_CH_2 + analogRead(A2);        // read value 9 times              

  voltage_CH_2  =  voltage_CH_2/2046 ;                     // calculate average value of 10 readings                                                            
}                                                          // 2046 = 10*1023/5V 
// -----------------------------------------------------------------------------------------------------
void  calculate_power_CH_2()        // used straight line equation: y = mmm*x + ccc -> x = (y - ccc)/mmm
{                                       
  level_CH_2 = -(voltage_CH_2-ccc)/mmm +att_CH2;
  level_CH_2 = floor(level_CH_2 + 0.5);                                // round and convert to integer
  power_W_2 = pow(10,level_CH_2/10);                                   // convert dBm to mW 
}

// -----------------------------------------------------------------------------------------------------
void select_subunit_of_power_CH_2()                                      // select unit: W, mW, µW, nW
{
  if(level_CH_2 <-30)
    { dtostrf(power_W_2*1000000,4,0,float_string);  //convert float to string
      oled.print(float_string);
      oled.print(" nW ");
    }

  if(level_CH_2 >= -30 && level_CH_2 <0)
    { dtostrf(power_W_2*1000,4,0,float_string);      
      oled.print(float_string);
      oled.print(" \xE4W ");                                // \xE4W = ascii code for µW
    } 

  if(level_CH_2 >=0 && level_CH_2 <30)
    { dtostrf(power_W_2,4,0,float_string);    
      oled.print(float_string);
      oled.print(" mW "); 
    } 

  if(level_CH_2 >=30 && level_CH_2 < 40)                // 1.0W to 9.9W
    { dtostrf(power_W_2/1000,4,1,float_string);    
      oled.print(float_string);
      oled.print(" W ");   
    } 

  if(level_CH_2 >=40 && level_CH_2 < 60)               // 10W to 999W
    { dtostrf(power_W_2/1000,4,0,float_string);    
      oled.print(float_string);
      oled.print(" W ");   
    }     

  if(level_CH_2 >= 60)
    { oled.clear();
      delay(100);
      oled.print(" Power 2P > 999W  "); 
      oled.setCursor(0,1);
      oled.print(" is out of range  ");
    }    
}
// =================================  Inversion des valeurs si le cablage est inversé  =========================== 
void calculate_ReturnLoss_and_SWR()
{   
  Return_Loss = level_CH_1 - level_CH_2;

  if(level_CH_1 < level_CH_2)
    { oled.clear();
      oled.print("!Level  CH2 >CH1");
      oled.setCursor(0,1);      
      oled.print("Change  Channels");
      delay(500);
    }
    
  RL_linear = pow(10,Return_Loss/20 );
  SWR = (RL_linear+1)/(RL_linear-1);
}

// ---------------------------------  Affichage du menu "Réfléchi"  --------------------------------------------------------------------
void display_power_CH_2()
{ 
  oled.clear();
  oled.set2X();
  oled.setCursor(0,0);                                                   // go to line #1
  dtostrf(level_CH_2,3,0,float_string);                                 //convert float to string
  oled.print(" ");   
  oled.print(float_string);
  oled.print(" dBm ");
      
  oled.setCursor(0,3);                                                   // go to line #2
  oled.print("Ref ");
  select_subunit_of_power_CH_2();

 oled.set1X();     
  oled.setCursor(0,6);    
  oled.print("ATT totale = ");  
  dtostrf(att_CH2,2,0,float_string);       
  oled.print(float_string);
  oled.print("dB ");
//  oled.write(byte(0));                                       // custom made char_up_down

  
  oled.setCursor(0,7);
  oled.print("Bde freq = ");
  dtostrf(f_ghz,3,1,float_string);   
  oled.print(float_string);
  oled.print(" GHz ");
//  oled.write(byte(1));                                       // custom made char_left_right

  delay(500);
  
  if((level_CH_2 - att_CH2) < error_limit_LOW || (level_CH_2 - att_CH2) > error_limit_HIGH)
      { delay(300); oled.setCursor(0,0); oled.print(" ");  oled.setCursor(0,1); oled.print(" ");delay(300);}
}


// ================================= Menu ROS-mètre ===========================================
void display_ReturnLoss_and_SWR()
{ 

  oled.clear();
 oled.set2X();

  oled.setCursor(0,0);                                                 // 1ère Ligne
  oled.print("Di ");   
  dtostrf(level_CH_1,3,0,float_string);                               //convert float to string   
  oled.print(float_string);
  oled.print("dBm ");
  
  oled.setCursor(0,3);                                    // 2ème Ligne
  oled.print("Re ");     
  dtostrf(level_CH_2,3,0,float_string);         //convert float to string 
  oled.print(float_string);
  oled.print("dBm ");

 
  oled.set1X();
  oled.setCursor(0,6);    
  oled.print("RL  =       ");
  dtostrf(Return_Loss,4,0,float_string);                              //convert float to string
  oled.print(float_string);
  oled.print(" dB");

  oled.setCursor(0,7);      
  oled.print("SWR =   ");
  dtostrf( SWR, 3, 1,float_string);  
  oled.print(float_string);

  delay(1000);		// Ralentissement affichage page 3 
}


// **************************************************************************************************************
// ================================= Loop ======================================================================= 
void loop()
{   
    if(display_menu_nr == 1)
    { 
      select_menu();                          // Choix du menu (1 à 3)sur oled-Keypad-Shield
      select_attenuator_CH_1();               // Choix attenuation totale (ligne de mesure + attenuateur additionnel)
      select_calibration_curve();             // Choix de la courbe d'étallonage correspond&ntz du AD8318 data sheet          
      read_output_CH_1();                     // Lecture du module AD8318
      calculate_power_CH_1();                 // Calcul de la puissance
      display_power_CH_1();                   // Affichage sur le oled-Keypad-Shield
    }

    if(display_menu_nr == 2)
    {
      select_menu();                          // 
      select_attenuator_CH_2();               // Idem (1)
      select_calibration_curve();             //   
      read_output_CH_2();                     // 
      calculate_power_CH_2();                 // 
      display_power_CH_2();                   // 
    }   

    if(display_menu_nr == 3)
    {
      read_output_CH_1();                      // Lecture module AD8318 CH1
      calculate_power_CH_1();                  // Calcul
      read_output_CH_2();                      // Lecture module AD8318 CH2
      calculate_power_CH_2();                  // Calcul
      calculate_ReturnLoss_and_SWR();
      display_ReturnLoss_and_SWR();            // Affichage du bilan des puissances
      select_menu();                           // et accès au menu      
     }

} 

// =====================================  Fin Loop  ===========================================================================
// ***************************************************************************************************************************

// ===================================== Pogramme Loop alternatif pour le paramètrage de la lecture des boutons ===============
// ====== Lecture et affichage sur l'afficheur OLED de la valeur rendue par les boutons 
//        pour le réglage des fourchettes d'identification des touches  ==========

/*
 void loop() {
   keyPress = analogRead(0)/10;
   oled.setCursor(0, 1);
  oled.print("in: ");
  oled.print(keyPress);
  oled.print(" ");

   if(keyPress < 6){
    oled.print("Right     ");
  } else if(keyPress < 22){
    oled.print("Up      ");
  } else if(keyPress < 39){
    oled.print("Down    ");
  } else if(keyPress < 60){
    oled.print("Left   ");
  } else if(keyPress < 87){
    oled.print("Select ");
  } else {
    oled.print("No btn");
  }
 }
 */
// ================================================================================================= END
