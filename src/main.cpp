// Module 5 - Step 1 : Restart contrôlé + raison du dernier démarrage
//
// Sur un device embarqué, redémarrer proprement est une stratégie de
// récupération courante : connexion perdue, capteur qui ne répond plus,
// état incohérent... plutôt que de rester bloqué, on reset et on repart.
//
// Ce step introduit deux outils complémentaires :
//
//   ESP.restart()       → redémarre l'ESP32 proprement (software reset)
//   esp_reset_reason()  → indique POURQUOI l'ESP32 a démarré
//                         (premier démarrage ? reset logiciel ? crash ?)
//
// La raison du dernier reset est très utile en debug terrain :
// si un device redémarre tout seul la nuit, tu sais pourquoi au matin.

#include <Arduino.h>

// Simule une condition d'erreur pour déclencher le restart
// Dans un vrai projet ce serait : timeout MQTT, capteur muet, etc.
#define BTN_RED 18  // Appui long → déclenche un restart volontaire

bool    btnPrev      = HIGH;
unsigned long btnPressedAt = 0;
#define LONG_PRESS_MS 2000UL // 2 secondes d'appui pour confirmer le restart

// ---------------------------------------------------------------------------
// resetReasonToString() : traduit le code de reset en texte lisible
//
// L'ESP32 mémorise la cause du dernier démarrage dans un registre matériel.
// esp_reset_reason() lit ce registre — il est disponible dès le setup().
// ---------------------------------------------------------------------------
const char *resetReasonToString(esp_reset_reason_t reason)
{
  switch (reason)
  {
    case ESP_RST_POWERON:   return "Mise sous tension";
    case ESP_RST_SW:        return "Reset logiciel";      // ESP.restart()
    case ESP_RST_PANIC:     return "Panic / crash";       // exception, nullptr...
    case ESP_RST_INT_WDT:   return "Watchdog interruption"; // step-2 !
    case ESP_RST_TASK_WDT:  return "Watchdog tache";       // step-2 !
    case ESP_RST_WDT:       return "Watchdog autre";
    case ESP_RST_DEEPSLEEP: return "Réveil deep sleep";
    case ESP_RST_BROWNOUT:  return "Sous-tension (brownout)";
    case ESP_RST_SDIO:      return "Reset SDIO";
    default:                return "Raison inconnue";
  }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  pinMode(BTN_RED, INPUT_PULLUP);

  // --- Lecture de la raison du dernier démarrage ---
  // À appeler le plus tôt possible dans setup() pour ne pas la perdre
  esp_reset_reason_t reason = esp_reset_reason();

  Serial.println("==================================");
  Serial.println("Firmware démarré");
  Serial.print("Raison du dernier reset : ");
  Serial.println(resetReasonToString(reason));
  Serial.println("==================================");

  // Comportement différent selon la raison du démarrage
  // → utile pour adapter l'initialisation (ex: ne pas réinitialiser la RAM
  //   si c'est un reset volontaire, mais le faire si c'est un crash)
  if (reason == ESP_RST_PANIC)
  {
    Serial.println("ATTENTION : le dernier démarrage était un crash !");
    Serial.println("Vérifie les logs pour trouver la cause.");
  }

  Serial.println("Maintiens le bouton rouge 2s pour déclencher un restart.");
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
  bool btnCurrent = digitalRead(BTN_RED);
  unsigned long now = millis();

  // Détecte le début d'un appui
  if (btnPrev == HIGH && btnCurrent == LOW)
  {
    btnPressedAt = now;
    Serial.println("Bouton maintenu... relâche pour annuler, tiens 2s pour restart.");
  }

  // Appui long atteint → restart
  if (btnCurrent == LOW && (now - btnPressedAt) >= LONG_PRESS_MS)
  {
    Serial.println("Restart volontaire dans 1 seconde...");
    Serial.flush(); // Vide le buffer série avant de couper — bonne pratique

    delay(1000);

    // ESP.restart() = reset logiciel propre
    // Après le reboot, esp_reset_reason() retournera ESP_RST_SW
    ESP.restart();
  }

  // Bouton relâché avant les 2s → annulé
  if (btnPrev == LOW && btnCurrent == HIGH)
  {
    if ((now - btnPressedAt) < LONG_PRESS_MS)
      Serial.println("Annulé.");
  }

  btnPrev = btnCurrent;
}
