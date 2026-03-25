// Module 5 - Step 3 : FreeRTOS — Les tâches (Tasks)
//
// Jusqu'ici tout ton code tournait dans une seule loop() séquentielle :
// une chose à la fois, dans l'ordre.
//
// FreeRTOS introduit le multitâche : plusieurs blocs de code qui tournent
// "en parallèle" sur l'ESP32. C'est comme des workers indépendants,
// chacun avec sa propre boucle et son propre rythme.
//
// Analogie web : pense à des async workers — chaque tâche est un worker
// qui tourne en fond sans bloquer les autres.
//
// Ce step crée 3 tâches indépendantes :
//   → Tâche 1 : clignote la LED verte toutes les 500ms
//   → Tâche 2 : clignote la LED rouge toutes les 1200ms (rythme différent)
//   → Tâche 3 : affiche un compteur sur Serial toutes les 2s
//
// Observe dans le Serial Monitor que les 3 rythmes sont bien indépendants —
// aucune tâche ne bloque les autres, même avec vTaskDelay().

#include <Arduino.h>

#define LED_GRN 2
#define LED_RED 5

// ---------------------------------------------------------------------------
// tacheLedVerte() : clignote la LED verte toutes les 500ms
//
// Signature imposée par FreeRTOS : retourne void, prend un void* en paramètre
// Le void* permet de passer des données à la tâche à sa création (on verra ça plus tard)
// ---------------------------------------------------------------------------
void tacheLedVerte(void *param)
{
  pinMode(LED_GRN, OUTPUT);

  // Les tâches FreeRTOS ont leur propre boucle infinie
  // Elles ne passent JAMAIS par loop()
  while (true)
  {
    digitalWrite(LED_GRN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(500)); // vTaskDelay = delay() non bloquant pour FreeRTOS
                                    // pdMS_TO_TICKS convertit des ms en "ticks" FreeRTOS
    digitalWrite(LED_GRN, LOW);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// ---------------------------------------------------------------------------
// tacheLedRouge() : clignote la LED rouge toutes les 1200ms
// ---------------------------------------------------------------------------
void tacheLedRouge(void *param)
{
  pinMode(LED_RED, OUTPUT);

  while (true)
  {
    digitalWrite(LED_RED, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1200));
    digitalWrite(LED_RED, LOW);
    vTaskDelay(pdMS_TO_TICKS(1200));
  }
}

// ---------------------------------------------------------------------------
// tacheSerial() : affiche un compteur toutes les 2 secondes
// ---------------------------------------------------------------------------
void tacheSerial(void *param)
{
  int compteur = 0;

  while (true)
  {
    Serial.print("Tâche Serial — tick : ");
    Serial.println(compteur++);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ---------------------------------------------------------------------------
// setup() : crée les tâches — elles démarrent immédiatement
// loop() ne sera presque plus utilisée dans un projet FreeRTOS
// ---------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  // xTaskCreatePinnedToCore() crée une tâche et la fixe sur un cœur CPU
  //
  // Paramètres :
  //   fonction        → le code de la tâche
  //   nom             → pour le debug (affiché dans les outils FreeRTOS)
  //   stack (bytes)   → mémoire allouée à la tâche (augmenter si crash "stack overflow")
  //   param           → données passées à la tâche (NULL ici, on verra plus tard)
  //   priorité        → 1 = basse, 5 = haute (0 réservé à idle)
  //   handle          → pointeur pour contrôler la tâche plus tard (NULL si pas besoin)
  //   cœur            → 0 ou 1 (l'ESP32 a 2 cœurs)

  xTaskCreatePinnedToCore(tacheLedVerte,  "LED Verte",  1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(tacheLedRouge,  "LED Rouge",  1024, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(tacheSerial,    "Serial",     2048, NULL, 1, NULL, 1);
  // Stack plus grande pour tacheSerial car Serial.print() consomme plus de mémoire

  Serial.println("3 tâches créées — observe les rythmes indépendants !");
}

// ---------------------------------------------------------------------------
// loop() : vide — les tâches FreeRTOS prennent le relais
// On garde quand même vTaskDelay pour ne pas monopoliser le CPU
// ---------------------------------------------------------------------------
void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
