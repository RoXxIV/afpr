#include <Arduino.h> // Inclusion de la bibliothèque Arduino

// Définition des constantes
#define LED_BLU 13 // LED bleue      → GPIO 13 (PWM)
#define POT     34 // Potentiomètre  → GPIO 34 (entrée analogique uniquement)

void setup()
{
  // PWM (Pulse Width Modulation) : au lieu d'envoyer HIGH ou LOW fixe,
  // on fait clignoter la broche très rapidement (1000 Hz).
  // En faisant varier le rapport cyclique (0% → éteint, 100% → plein),
  // l'œil perçoit une luminosité variable.
  //
  // ledcAttach(broche, fréquence Hz, résolution en bits)
  // 8 bits = 2^8 = 256 niveaux possibles (0 à 255)
  ledcAttach(LED_BLU, 1000, 8);
}

void loop()
{
  // analogRead() lit la tension sur la broche du potentiomètre (0V à 3.3V)
  // et retourne un entier sur 12 bits :
  //   0    = 0V   (potentiomètre à fond à gauche)
  //   4095 = 3.3V (potentiomètre à fond à droite)
  int potValue = analogRead(POT);

  // map() convertit une valeur d'une plage vers une autre :
  //   map(valeur, min_entrée, max_entrée, min_sortie, max_sortie)
  // Ici : ADC 12 bits (0-4095) → PWM 8 bits (0-255)
  int pwmValue = map(potValue, 0, 4095, 0, 255);

  // ledcWrite envoie le signal PWM sur la broche
  //   pwmValue = 0   → 0% rapport cyclique  → LED éteinte
  //   pwmValue = 255 → 100% rapport cyclique → LED pleine intensité
  ledcWrite(LED_BLU, pwmValue);
}
