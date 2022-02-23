# Wattmetre_Teensy4

Création d'un wattmètre UHF et SHF utilisant:
- des lignes de mesures coaxiales, 
- des AD8318 https://www.analog.com/en/products/ad8318.html
- "motorisé" un Teensy_4.0 https://www.pjrc.com/store/teensy40.html 
- associé à un afficheur OLED 1.3" en version I2C.
- avec 5 touches cablées comme sur les LCD_Keypad_Shield

Entrée "Select" pour choisir le menu 1 2 ou 3 (Direct, Réfléchi ou ROS)
Entrée "Haut" et "Bas" pour choisir l'atténuation totale (ligne de mesure + l'atténuateur d'adaptation)
Puissance d'entrée utile sur un AD4318 0dBm et puissance max sur son entrée 12 dBm

 Exemple: pour mesurer une puissance maxi de 100W il faut atténuer de 50 dB pour avoir 0dBm soit 1mW à mesurer
          avec une ligne atténuant de 30dB, il faudra rajouter un bouchon de 20 dB pour avoir une mesure correcte
          Ligne -30 dB + Bouchon -20dB   =   "Atténuation totale" -50dB
          Je peux envoyer 100W soit 50dBm pour une mesure à 0dBm soit 1mW
          
- Soit avec ces combinaisons:
-    Ligne   Bouchon   P Max Mesurable
// ---------------------------------------
//    30 dB    sans      1 W = 30 dBm
//    30 dB    10 dB    10 W = 40 dBm
//    30 dB    20 dB   100 W = 50 dBm
//    30 dB    23 dB   200 W = 53 dBm           Les lignes étant limitées à 200 W, il faut s'arrêter là.
// 
//    Ligne   Bouchon   P Max Mesurable
// ---------------------------------------
//    20 dB    sans    100 mW = 20 dBm
//    20 dB    10 dB     1 W = 30 dBm
//    20 dB    20 dB    10 W = 40 dBm
//    20 dB    30 dB   100 W = 50 dBm 

Le logiciel se paramètre pour lui indiquer l'Atténuation Totale et la Fréquence, pour afficher directement les valeurs de 
  Puissance Directe,
  Puissance réfléchie, 
  Return Loss et ROS

Le tout écrit pour édition par Visual Studio Code avec PlatfomIO
