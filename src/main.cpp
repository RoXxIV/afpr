#include <Arduino.h> // Inclusion de la bibliothèque Arduino

// Définition des constantes
#define LED_RED 5  // LED rouge  → GPIO 5
#define BTN_RED 18 // Bouton rouge → GPIO 18

void setup()
{
  // Configure la broche LED en sortie
  pinMode(LED_RED, OUTPUT);

  // Configure la broche bouton en entrée avec résistance de tirage interne (PULLUP)
  // → 1 (HIGH) au repos, 0 (LOW) quand le bouton est pressé
  pinMode(BTN_RED, INPUT_PULLUP);
}

void loop()
{
  // Lit l'état du bouton : LOW si pressé, HIGH sinon
  if (digitalRead(BTN_RED) == LOW)
    digitalWrite(LED_RED, HIGH); // Bouton pressé → allume la LED
  else
    digitalWrite(LED_RED, LOW); // Bouton relâché → éteint la LED
}
