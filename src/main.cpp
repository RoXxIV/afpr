// Module 5 - Step 4 : FreeRTOS — Les Queues
//
// Au step-3, les tâches tournaient en parallèle mais de façon isolée.
// Comment une tâche envoie-t-elle une information à une autre ?
//
// Mauvaise idée : partager une variable globale directement.
//   → Risque de "race condition" : deux tâches lisent/écrivent en même temps
//   → On verra comment protéger ça au step-5 (Mutex)
//
// Bonne idée : utiliser une Queue (file d'attente).
//   → La tâche A ENVOIE un message dans la queue
//   → La tâche B REÇOIT le message quand elle est prête
//   → FreeRTOS garantit que c'est thread-safe
//
// Analogie web : c'est exactement un EventEmitter ou un message bus —
// les tâches ne se connaissent pas, elles parlent via un canal partagé.
//
// Ce step simule un pipeline simple :
//   Tâche Bouton  → détecte un appui → envoie l'événement dans la queue
//   Tâche LED     → reçoit l'événement → allume/éteint la LED

#include <Arduino.h>

#define BTN_GRN 16
#define BTN_YLW 19
#define LED_GRN 2
#define LED_YLW 4

// --- Types de messages échangés via la queue ---
// Un enum rend le code lisible : on envoie un EVENT, pas un int brut
typedef enum {
  EVT_BTN_GRN,  // bouton vert pressé
  EVT_BTN_YLW,  // bouton jaune pressé
} BoutonEvent;

// --- Handle de la queue ---
// QueueHandle_t est le "pointeur" vers la queue — partagé entre les tâches
// Déclaré global pour être accessible par toutes les tâches
QueueHandle_t queueBoutons;

// ---------------------------------------------------------------------------
// tacheBoutons() : surveille les boutons et envoie des événements
// Producteur — ne sait pas ce qui consomme la queue
// ---------------------------------------------------------------------------
void tacheBoutons(void *param)
{
  pinMode(BTN_GRN, INPUT_PULLUP);
  pinMode(BTN_YLW, INPUT_PULLUP);

  bool prevGrn = HIGH;
  bool prevYlw = HIGH;

  while (true)
  {
    bool curGrn = digitalRead(BTN_GRN);
    bool curYlw = digitalRead(BTN_YLW);

    // Front descendant bouton vert → envoie l'événement dans la queue
    if (prevGrn == HIGH && curGrn == LOW)
    {
      BoutonEvent evt = EVT_BTN_GRN;

      // xQueueSend() place le message dans la queue
      // pdMS_TO_TICKS(0) = n'attend pas si la queue est pleine (non bloquant)
      // Si la queue est pleine, le message est perdu — acceptable pour des boutons
      xQueueSend(queueBoutons, &evt, pdMS_TO_TICKS(0));
    }

    if (prevYlw == HIGH && curYlw == LOW)
    {
      BoutonEvent evt = EVT_BTN_YLW;
      xQueueSend(queueBoutons, &evt, pdMS_TO_TICKS(0));
    }

    prevGrn = curGrn;
    prevYlw = curYlw;

    vTaskDelay(pdMS_TO_TICKS(20)); // Anti-rebond via délai court
  }
}

// ---------------------------------------------------------------------------
// tacheLeds() : reçoit les événements et pilote les LEDs
// Consommateur — ne sait pas qui produit dans la queue
// ---------------------------------------------------------------------------
void tacheLeds(void *param)
{
  pinMode(LED_GRN, OUTPUT);
  pinMode(LED_YLW, OUTPUT);

  bool etatGrn = false;
  bool etatYlw = false;

  BoutonEvent evt;

  while (true)
  {
    // xQueueReceive() attend qu'un message arrive dans la queue
    // portMAX_DELAY = attend indéfiniment (la tâche dort jusqu'à réception)
    // → pas de CPU consommé pendant l'attente, contrairement à un polling
    if (xQueueReceive(queueBoutons, &evt, portMAX_DELAY) == pdTRUE)
    {
      switch (evt)
      {
        case EVT_BTN_GRN:
          etatGrn = !etatGrn;
          digitalWrite(LED_GRN, etatGrn);
          Serial.println(etatGrn ? "LED verte  → ON" : "LED verte  → OFF");
          break;

        case EVT_BTN_YLW:
          etatYlw = !etatYlw;
          digitalWrite(LED_YLW, etatYlw);
          Serial.println(etatYlw ? "LED jaune  → ON" : "LED jaune  → OFF");
          break;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);

  // Crée la queue AVANT de créer les tâches qui vont l'utiliser
  // Paramètres :
  //   longueur  → nombre max de messages en attente (ici 10)
  //   taille    → taille d'un message en bytes (sizeof notre enum)
  queueBoutons = xQueueCreate(10, sizeof(BoutonEvent));

  if (queueBoutons == NULL)
  {
    Serial.println("ERREUR : impossible de créer la queue !");
    return;
  }

  xTaskCreatePinnedToCore(tacheBoutons, "Boutons", 2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(tacheLeds,    "LEDs",    2048, NULL, 1, NULL, 0);
  // Boutons a une priorité plus haute (2) que LEDs (1)
  // → garantit que la détection d'appui n'est jamais retardée par la tâche LED

  Serial.println("Queue créée — appuie sur les boutons !");
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
