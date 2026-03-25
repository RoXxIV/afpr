// Module 5 - Step 8 : Multi-fichier C++
//
// Jusqu'ici tout le code tenait dans main.cpp.
// Sur un vrai projet, c'est ingérable : 500, 1000, 2000 lignes dans un seul fichier.
//
// La solution : découper en modules indépendants, chacun dans son propre dossier.
// C'est exactement l'architecture du vrai projet :
//
//   lib/DisplayManager/DisplayManager.h + .cpp
//   lib/MenuManager/MenuManager.h + .cpp
//   lib/BatteryLogic/BatteryLogic.h + .cpp
//   ...
//
// Ce step crée un premier module : LedManager
//   lib/LedManager/LedManager.h   → déclaration (quoi)
//   lib/LedManager/LedManager.cpp → implémentation (comment)
//
// PlatformIO découvre automatiquement les fichiers dans lib/ — pas de config à faire.
//
// Ouvre lib/LedManager/LedManager.h et LedManager.cpp avant de lire ce fichier.

#include <Arduino.h>
#include <LedManager.h>  // PlatformIO cherche dans lib/ automatiquement

#define BTN_GRN 16
#define BTN_YLW 19

// Instanciation : crée un objet LedManager pour chaque LED
// On passe le GPIO au constructeur — comme new LedManager(pin) en JS
LedManager ledVerte(2);
LedManager ledJaune(4);

bool btnGrnPrev = HIGH;
bool btnYlwPrev = HIGH;

void setup()
{
  Serial.begin(115200);
  pinMode(BTN_GRN, INPUT_PULLUP);
  pinMode(BTN_YLW, INPUT_PULLUP);

  // begin() initialise le GPIO — séparé du constructeur car pinMode()
  // ne peut pas être appelé avant que le framework Arduino soit initialisé
  ledVerte.begin();
  ledJaune.begin();

  // Clignote 3 fois au démarrage pour signaler que tout est prêt
  ledVerte.blink(3, 100);

  Serial.println("LedManager prêt !");
  Serial.println("BTN VERT  → toggle LED verte");
  Serial.println("BTN JAUNE → toggle LED jaune + état dans Serial");
}

void loop()
{
  bool btnGrn = digitalRead(BTN_GRN);
  bool btnYlw = digitalRead(BTN_YLW);

  if (btnGrnPrev == HIGH && btnGrn == LOW)
  {
    ledVerte.toggle();
    delay(50);
  }

  if (btnYlwPrev == HIGH && btnYlw == LOW)
  {
    ledJaune.toggle();

    // isOn() : accès à l'état sans toucher directement à la variable interne
    // → c'est l'encapsulation — main.cpp ne connaît pas _state, il demande à l'objet
    Serial.print("LED jaune : ");
    Serial.println(ledJaune.isOn() ? "ON" : "OFF");
    delay(50);
  }

  btnGrnPrev = btnGrn;
  btnYlwPrev = btnYlw;
}
