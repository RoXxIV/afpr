#pragma once

#include <Arduino.h>

// ButtonManager : gestion d'un bouton avec anti-rebond intégré
//
// Chaque bouton est un objet indépendant — propre et réutilisable.
// Dans le vrai projet, ButtonManager gère aussi les appuis longs
// et les événements pour le menu (next, prev, select).

class ButtonManager
{
public:
  ButtonManager(int pin);
  void begin();

  // À appeler dans loop() ou dans une tâche FreeRTOS
  // Retourne true si un appui court vient d'être détecté (front descendant validé)
  bool pressed();

private:
  int           _pin;
  bool          _prev;
  unsigned long _lastDebounce;
  bool          _handled;
};
