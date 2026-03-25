// Module 5 - Step 5 : FreeRTOS — Mutex
//
// Au step-4, les tâches communiquaient via une queue — propre et thread-safe.
// Mais parfois deux tâches ont besoin d'accéder à la MÊME ressource
// (un écran LCD, le port Serial, une variable partagée...).
//
// Problème : si les deux écrivent en même temps → données corrompues.
// C'est ce qu'on appelle une "race condition".
//
// Analogie web : deux requêtes qui modifient la même ligne en base de données
// en même temps sans transaction → résultat imprévisible.
//
// La solution : le Mutex (Mutual Exclusion — exclusion mutuelle).
//   → Une seule tâche à la fois peut "prendre" le mutex
//   → Les autres attendent qu'il soit "rendu" avant de continuer
//   → C'est un verrou sur une ressource partagée
//
// Ce step démontre le problème EN DEUX TEMPS :
//   Phase 1 (SANS_MUTEX = 1) : deux tâches écrivent sur Serial en même temps
//                               → observe le texte mélangé/corrompu
//   Phase 2 (SANS_MUTEX = 0) : le mutex protège Serial
//                               → les messages s'affichent proprement
//
// Change la valeur de SANS_MUTEX, recompile, et compare.

#include <Arduino.h>

// Passe à 0 pour activer la protection par mutex
#define SANS_MUTEX 1

SemaphoreHandle_t mutexSerial;

// ---------------------------------------------------------------------------
// ecrireSerial() : écrit un message multiligne sur Serial
// Sans mutex, si deux tâches appellent cette fonction en même temps,
// leurs sorties vont s'entremêler
// ---------------------------------------------------------------------------
void ecrireSerial(const char *nomTache, int compteur)
{
#if SANS_MUTEX == 0
  // xSemaphoreTake() : demande le verrou
  // pdMS_TO_TICKS(100) : attend max 100ms — si pas libre → abandonne
  if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(100)) == pdTRUE)
  {
#endif

    // Bloc critique : une seule tâche à la fois exécute ce code
    Serial.print("[ ");
    Serial.print(nomTache);
    Serial.print(" ] message ");
    Serial.print(compteur);
    Serial.print(" — millis: ");
    Serial.println(millis());

#if SANS_MUTEX == 0
    // xSemaphoreGive() : rend le verrou — la prochaine tâche en attente peut continuer
    xSemaphoreGive(mutexSerial);
  }
#endif
}

// ---------------------------------------------------------------------------
// tacheA() et tacheB() : deux tâches qui écrivent sur Serial simultanément
// Elles ont la même priorité et tournent sur le même cœur → préemption possible
// ---------------------------------------------------------------------------
void tacheA(void *param)
{
  int compteur = 0;
  while (true)
  {
    ecrireSerial("Tache-A", compteur++);
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

void tacheB(void *param)
{
  int compteur = 0;
  while (true)
  {
    ecrireSerial("Tache-B", compteur++);
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);

#if SANS_MUTEX == 1
  Serial.println("=== MODE SANS MUTEX — observe le texte corrompu ===");
  Serial.println("Change SANS_MUTEX à 0 et recompile pour voir la différence.");
#else
  Serial.println("=== MODE AVEC MUTEX — les messages sont propres ===");
#endif

  // Crée le mutex AVANT les tâches
  // xSemaphoreCreateMutex() retourne NULL si plus de mémoire disponible
  mutexSerial = xSemaphoreCreateMutex();

  if (mutexSerial == NULL)
  {
    Serial.println("ERREUR : impossible de créer le mutex !");
    return;
  }

  // Même cœur (0) et même priorité (1) → FreeRTOS les préempte l'une l'autre
  // C'est volontaire : maximise les chances de race condition sans mutex
  xTaskCreatePinnedToCore(tacheA, "Tache-A", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(tacheB, "Tache-B", 2048, NULL, 1, NULL, 0);
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
