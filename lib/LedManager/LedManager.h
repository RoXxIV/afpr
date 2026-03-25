#pragma once  // Empêche l'inclusion multiple du même header
              // Alternative à #ifndef LEDMANAGER_H / #define / #endif
              // Plus moderne et plus simple — préfère toujours #pragma once

// LedManager.h — DÉCLARATION de la classe
//
// En C++, le fichier .h contient les déclarations :
//   → quelles méthodes existent, quels paramètres elles prennent
//   → quelles variables la classe possède
//
// Le fichier .cpp contient les IMPLÉMENTATIONS (le vrai code).
//
// Analogie web : le .h c'est comme une interface TypeScript,
// le .cpp c'est l'implémentation de cette interface.
//
// Pourquoi séparer ?
//   → Plusieurs fichiers peuvent inclure le .h sans dupliquer le code
//   → Le compilateur peut vérifier les types sans tout recompiler
//   → C'est la base de l'architecture modulaire du vrai projet

#include <Arduino.h>

class LedManager
{
public:
  // Constructeur : appelé à la création de l'objet
  // Prend le numéro de broche GPIO en paramètre
  LedManager(int pin);

  // Initialise la broche — à appeler dans setup()
  void begin();

  // Allume / éteint la LED
  void on();
  void off();

  // Inverse l'état actuel
  void toggle();

  // Retourne true si la LED est allumée
  bool isOn();

  // Fait clignoter la LED N fois
  void blink(int fois, int dureeMs);

private:
  // Les membres "private" ne sont accessibles que depuis l'intérieur de la classe
  // → encapsulation : main.cpp ne peut pas modifier _pin ou _state directement
  int  _pin;
  bool _state;
};
