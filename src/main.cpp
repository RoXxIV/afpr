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
//   Phase 1 (SANS_MUTEX = 1) : deux tâches écrivent sur le LCD en même temps
//                               → observe le texte mélangé/corrompu sur la ligne 1
//   Phase 2 (SANS_MUTEX = 0) : le mutex protège le LCD
//                               → les messages s'affichent proprement
//
// Change la valeur de SANS_MUTEX, recompile, et compare.

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Passe à 0 pour activer la protection par mutex
#define SANS_MUTEX 0

SemaphoreHandle_t mutexLCD;

// ---------------------------------------------------------------------------
// ecrireLCD() : écrit sur le LCD
// Sans mutex, si deux tâches appellent cette fonction en même temps,
// leurs écritures s'entremêlent sur l'écran
// ---------------------------------------------------------------------------
void ecrireLCD(const char *nomTache, int compteur)
{
#if SANS_MUTEX == 0
  // xSemaphoreTake() : demande le verrou
  // pdMS_TO_TICKS(100) : attend max 100ms — si pas libre → abandonne
  if (xSemaphoreTake(mutexLCD, portMAX_DELAY) == pdTRUE)
  {
#endif

    // Bloc critique : une seule tâche à la fois exécute ce code
    // Le vTaskDelay entre setCursor et print garantit la préemption
    // → sans mutex : l'autre tâche s'intercale pendant le délai → texte corrompu
    // → avec mutex : l'autre tâche attend que le verrou soit rendu
    lcd.setCursor(0, 1); // ligne 2 mise à jour immédiatement
    lcd.print("msg:            ");
    lcd.setCursor(4, 1);
    lcd.print(compteur);
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    vTaskDelay(pdMS_TO_TICKS(800)); // fenêtre de vulnérabilité sur la ligne 1
    lcd.print(nomTache);
    vTaskDelay(pdMS_TO_TICKS(1500)); // laisse le temps de lire le résultat

    Serial.print("[ ");
    Serial.print(nomTache);
    Serial.print(" ] message ");
    Serial.print(compteur);
    Serial.print(" — millis: ");
    Serial.println(millis());

#if SANS_MUTEX == 0
    // xSemaphoreGive() : rend le verrou — la prochaine tâche en attente peut continuer
    xSemaphoreGive(mutexLCD);
  }
#endif
}

// ---------------------------------------------------------------------------
// tacheA() et tacheB() : deux tâches qui écrivent sur le LCD simultanément
// Elles ont la même priorité et tournent sur le même cœur → préemption possible
// ---------------------------------------------------------------------------
void tacheA(void *param)
{
  int compteur = 0;
  while (true)
  {
    ecrireLCD("Tache-A", compteur++);
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

void tacheB(void *param)
{
  int compteur = 0;
  while (true)
  {
    ecrireLCD("Tache-B", compteur++);
    vTaskDelay(pdMS_TO_TICKS(300));
  }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

#if SANS_MUTEX == 1
  Serial.println("=== MODE SANS MUTEX — observe le texte corrompu sur le LCD ===");
  Serial.println("Change SANS_MUTEX à 0 et recompile pour voir la différence.");
#else
  Serial.println("=== MODE AVEC MUTEX — le LCD s'affiche proprement ===");
#endif

  // Crée le mutex AVANT les tâches
  // xSemaphoreCreateMutex() retourne NULL si plus de mémoire disponible
  mutexLCD = xSemaphoreCreateMutex();

  if (mutexLCD == NULL)
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
