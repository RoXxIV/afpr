// Module 5 - Step 2 : Task Watchdog — le filet de sécurité matériel
//
// Au step-1, c'était TOI qui décidais de redémarrer avec ESP.restart().
// Ici c'est l'inverse : tu délègues la surveillance au hardware.
//
// Principe du watchdog :
//   → Tu enregistres ta tâche auprès du watchdog
//   → Tu dois appeler esp_task_wdt_reset() régulièrement ("nourrir le chien")
//   → Si tu ne le fais pas dans le délai imparti (ex: code bloqué, boucle infinie)
//     le watchdog reset l'ESP32 tout seul, sans que ton code intervienne
//
// C'est le filet ultime : il protège contre les situations où ton code
// ne peut PLUS décider de redémarrer lui-même.
//
// Ce step simule deux scénarios :
//   Bouton VERT  → tout va bien, le watchdog est nourri normalement
//   Bouton ROUGE → simule un freeze (delay bloquant) → watchdog se déclenche
//                  Au reboot : esp_reset_reason() retourne ESP_RST_TASK_WDT

#include <Arduino.h>
#include <esp_task_wdt.h>  // API watchdog de l'ESP32 (intégrée au framework)

#define BTN_GRN 16  // Nourrit le watchdog manuellement (mode nominal)
#define BTN_RED 18  // Simule un freeze → déclenche le watchdog

#define WDT_TIMEOUT_S 5  // Le watchdog reset l'ESP si pas nourri pendant 5 secondes

// ---------------------------------------------------------------------------
// resetReasonToString() : repris du step-1
// Tu vas maintenant voir ESP_RST_TASK_WDT apparaître après un freeze
// ---------------------------------------------------------------------------
const char *resetReasonToString(esp_reset_reason_t reason)
{
  switch (reason)
  {
    case ESP_RST_POWERON:  return "Mise sous tension";
    case ESP_RST_SW:       return "Reset logiciel (ESP.restart)";
    case ESP_RST_PANIC:    return "Panic / crash";
    case ESP_RST_INT_WDT:  return "Watchdog interruption";
    case ESP_RST_TASK_WDT: return "Watchdog tâche ← c'est ici !";  // Ce step !
    case ESP_RST_WDT:      return "Watchdog autre";
    default:               return "Autre";
  }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  pinMode(BTN_GRN, INPUT_PULLUP);
  pinMode(BTN_RED, INPUT_PULLUP);

  // Affiche la raison du dernier démarrage — si le watchdog s'est déclenché
  // tu verras "Watchdog tâche ← c'est ici !" au reboot
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.println("==================================");
  Serial.print("Raison du dernier reset : ");
  Serial.println(resetReasonToString(reason));
  Serial.println("==================================");

  // --- Initialisation du watchdog ---
  //
  // esp_task_wdt_config_t définit le comportement du watchdog :
  //   timeout_ms   → délai avant reset si le chien n'est pas nourri
  //   trigger_panic → true = génère un panic avant le reset (plus de logs)
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms     = WDT_TIMEOUT_S * 1000,
    .idle_core_mask = 0,
    .trigger_panic  = false  // false = reset direct, true = panic + stack trace
  };
  esp_task_wdt_reconfigure(&wdt_config);

  // Enregistre la tâche courante (loop) auprès du watchdog
  // À partir de maintenant, cette tâche DOIT appeler esp_task_wdt_reset()
  // toutes les WDT_TIMEOUT_S secondes, sinon → reset
  esp_task_wdt_add(NULL); // NULL = tâche courante (loop)

  Serial.println("Watchdog actif — timeout : " + String(WDT_TIMEOUT_S) + "s");
  Serial.println("BTN VERT  → nourrit le watchdog");
  Serial.println("BTN ROUGE → simule un freeze (watchdog se déclenche dans " + String(WDT_TIMEOUT_S) + "s)");
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
  // --- Nourrir le watchdog ---
  // Dans un vrai projet, cet appel est placé dans la boucle principale.
  // Si la boucle tourne normalement, le chien est nourri et rien ne se passe.
  // Si elle se bloque, le chien n'est plus nourri → reset.
  esp_task_wdt_reset();

  Serial.println("Watchdog nourri — système OK");

  // BTN ROUGE : simule un freeze avec un delay bloquant
  // Le watchdog ne sera plus nourri → reset au bout de WDT_TIMEOUT_S secondes
  if (digitalRead(BTN_RED) == LOW)
  {
    Serial.println("FREEZE simulé ! Watchdog déclenché dans " + String(WDT_TIMEOUT_S) + "s...");
    Serial.flush();
    delay(WDT_TIMEOUT_S * 1000 + 1000); // Bloque volontairement au-delà du timeout
    // Cette ligne ne sera jamais atteinte — le watchdog reset avant
    Serial.println("(jamais affiché)");
  }

  // BTN VERT : nourrit manuellement le watchdog et affiche un message
  // → permet de tester que le watchdog ne se déclenche PAS quand tout va bien
  if (digitalRead(BTN_GRN) == LOW)
  {
    Serial.println("Bouton vert pressé — watchdog nourri manuellement");
    delay(50); // anti-rebond (court, dans le délai du watchdog)
  }

  delay(1000); // Simule une boucle qui prend 1s — bien en dessous du timeout de 5s
}
