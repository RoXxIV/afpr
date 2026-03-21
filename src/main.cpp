#include <Arduino.h> // Inclusion de la bibliothèque Arduino

// Définition des constantes
#define LED_YLW 4  // LED jaune  → GPIO 4
#define BTN_YLW 19 // Bouton jaune → GPIO 19

// Variable globale pour mémoriser l'état de la LED jaune (allumée ou éteinte)
// "bool" = booléen, ne peut valoir que true (vrai) ou false (faux)
bool ylwLedState = false;

// Variable pour mémoriser l'état précédent du bouton jaune
// Permet de détecter le moment exact où le bouton est pressé (front descendant)
bool ylwBtnPrev = HIGH;

void setup()
{
  // Configure la broche LED en sortie
  pinMode(LED_YLW, OUTPUT);

  // Configure la broche bouton en entrée avec résistance de tirage interne (PULLUP)
  // → 1 (HIGH) au repos, 0 (LOW) quand le bouton est pressé
  pinMode(BTN_YLW, INPUT_PULLUP);
}

void loop()
{
  // Lit l'état actuel du bouton jaune
  bool ylwBtnCurrent = digitalRead(BTN_YLW);

  // Détecte le front descendant : le bouton vient d'être pressé
  // (il était HIGH au tour précédent et il est maintenant LOW)
  if (ylwBtnPrev == HIGH && ylwBtnCurrent == LOW)
  {
    // Inverse l'état de la LED : allumée → éteinte, ou éteinte → allumée
    ylwLedState = !ylwLedState;

    // Applique le nouvel état sur la broche
    digitalWrite(LED_YLW, ylwLedState ? HIGH : LOW);

    // Anti-rebond : attend 50ms pour ignorer les faux contacts du bouton
    // Un bouton mécanique génère plusieurs transitions rapides lors d'un appui
    delay(50);
  }

  // Sauvegarde l'état du bouton pour le prochain tour de boucle
  ylwBtnPrev = ylwBtnCurrent;
}
