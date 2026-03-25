#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// DisplayManager : wrapper autour du LCD
//
// Centralise toutes les opérations d'affichage.
// MenuManager appelle DisplayManager — il ne touche jamais le LCD directement.
// Même pattern que dans le vrai projet avec U8g2.

class DisplayManager
{
public:
  DisplayManager(uint8_t address, uint8_t cols, uint8_t rows);
  void begin();

  void clear();

  // Affiche une ligne complète (efface le reste de la ligne automatiquement)
  void printLine(uint8_t row, const String &text);

  // Affiche deux lignes d'un coup
  void printScreen(const String &line0, const String &line1);

private:
  LiquidCrystal_I2C _lcd;
  uint8_t           _cols;
};
