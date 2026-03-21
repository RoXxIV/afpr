#include <Arduino.h> // Inclusion de la bibliothèque Arduino

// Définition des constantes
#define LED_GRN 2 // LED verte → GPIO 2

// Variables pour le clignotement non-bloquant
// On utilise "unsigned long" car millis() peut atteindre de grandes valeurs (max ~49 jours)
bool grnLedState = false;        // État actuel de la LED (allumée ou éteinte)
unsigned long grnLastToggle = 0; // Timestamp du dernier changement d'état (en ms)

void setup()
{
  // Configure la broche LED en sortie
  pinMode(LED_GRN, OUTPUT);
}

void loop()
{
  // --- Pourquoi ne pas utiliser delay(1000) ? ---
  // delay() BLOQUE tout le programme pendant 1 seconde.
  // Avec millis(), la loop() continue de tourner librement :
  // on vérifie juste "est-ce que 1000ms se sont écoulées ?"
  //
  // Principe : chronomètre sans blocage
  //   temps actuel - temps du dernier toggle >= 1000ms → c'est l'heure de basculer

  unsigned long maintenant = millis(); // Retourne le nombre de ms depuis le démarrage

  if (maintenant - grnLastToggle >= 1000) // 1000 ms = 1 seconde écoulée ?
  {
    grnLastToggle = maintenant;          // Réinitialise le chronomètre
    grnLedState = !grnLedState;          // Bascule l'état de la LED
    digitalWrite(LED_GRN, grnLedState); // Applique l'état sur la broche
  }
}
