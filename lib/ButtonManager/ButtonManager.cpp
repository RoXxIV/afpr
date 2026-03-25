#include "ButtonManager.h"

#define DEBOUNCE_MS 50

ButtonManager::ButtonManager(int pin)
{
  _pin         = pin;
  _prev        = HIGH;
  _lastDebounce = 0;
  _handled     = false;
}

void ButtonManager::begin()
{
  pinMode(_pin, INPUT_PULLUP);
}

bool ButtonManager::pressed()
{
  bool cur = digitalRead(_pin);
  unsigned long now = millis();

  // Front descendant → démarre le chronomètre anti-rebond
  if (_prev == HIGH && cur == LOW)
  {
    _lastDebounce = now;
    _handled      = false;
  }

  // Signal stable depuis DEBOUNCE_MS et pas encore traité → appui validé
  bool result = false;
  if (!_handled && cur == LOW && (now - _lastDebounce) >= DEBOUNCE_MS)
  {
    _handled = true;
    result   = true;
  }

  // Front montant → prêt pour un nouvel appui
  if (_prev == LOW && cur == HIGH)
    _handled = false;

  _prev = cur;
  return result;
}
