// LedManager.cpp — IMPLÉMENTATION de la classe
//
// Ce fichier contient le vrai code des méthodes déclarées dans LedManager.h
// On inclut toujours le .h correspondant en premier.

#include "LedManager.h"

// --- Constructeur ---
// LedManager:: indique que cette fonction appartient à la classe LedManager
// C'est la syntaxe C++ pour implémenter une méthode en dehors de sa déclaration
LedManager::LedManager(int pin)
{
  _pin   = pin;
  _state = false;
}

void LedManager::begin()
{
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
}

void LedManager::on()
{
  _state = true;
  digitalWrite(_pin, HIGH);
}

void LedManager::off()
{
  _state = false;
  digitalWrite(_pin, LOW);
}

void LedManager::toggle()
{
  _state = !_state;
  digitalWrite(_pin, _state);
}

bool LedManager::isOn()
{
  return _state;
}

void LedManager::blink(int fois, int dureeMs)
{
  bool etatInitial = _state;

  for (int i = 0; i < fois; i++)
  {
    on();
    delay(dureeMs);
    off();
    delay(dureeMs);
  }

  // Restaure l'état d'avant le clignotement
  if (etatInitial) on();
}
