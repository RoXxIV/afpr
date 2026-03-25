#include "DisplayManager.h"

DisplayManager::DisplayManager(uint8_t address, uint8_t cols, uint8_t rows)
  : _lcd(address, cols, rows), _cols(cols)
{
  // La liste d'initialisation (: _lcd(...)) initialise les membres
  // avant l'entrée dans le corps du constructeur — nécessaire pour les objets
}

void DisplayManager::begin()
{
  _lcd.init();
  _lcd.backlight();
}

void DisplayManager::clear()
{
  _lcd.clear();
}

void DisplayManager::printLine(uint8_t row, const String &text)
{
  _lcd.setCursor(0, row);

  // Tronque ou complète avec des espaces pour remplir la ligne
  // → évite les artefacts de l'affichage précédent
  String padded = text;
  while (padded.length() < _cols)
    padded += ' ';

  _lcd.print(padded.substring(0, _cols));
}

void DisplayManager::printScreen(const String &line0, const String &line1)
{
  printLine(0, line0);
  printLine(1, line1);
}
