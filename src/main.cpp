// Module 5 - Step 6 : FreeRTOS — Intégration (architecture production)
//
// Ce step assemble tout ce que tu as vu :
//   Step-3 : Tasks     → chaque module tourne dans sa propre tâche
//   Step-4 : Queues    → les boutons envoient des événements
//   Step-5 : Mutex     → SharedData protégé contre les accès simultanés
//   Module-5/Step-2    → Watchdog surveillant chaque tâche
//
// L'architecture est calquée sur le vrai projet multi-bat-freertos-solis.
// À la fin de ce step, ouvre include/SharedData.h du vrai projet —
// tu reconnaîtras exactement la même structure.
//
// Architecture :
//
//   [Tâche Sensors] ──write──> [SharedData + Mutex] <──read── [Tâche Display]
//                                                    <──read── [Tâche MQTT]
//
//   Chaque tâche nourrit le watchdog — si l'une freeze, l'ESP32 reboot.

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <esp_task_wdt.h>

// --- Config ---
const char *ssid       = "NovaHome";
const char *password   = "nova1234";
const char *mqttBroker = "192.168.1.80";
#define WDT_TIMEOUT_S 10

// --- GPIO ---
#define BTN_GRN 16
#define LED_GRN 2

// ===========================================================================
// SharedData : structure de données partagée entre toutes les tâches
//
// Dans le vrai projet, c'est un fichier dédié : include/SharedData.h
// Ici on la déclare directement pour garder un seul fichier pédagogique.
//
// RÈGLE ABSOLUE : on n'accède JAMAIS à SharedData sans prendre le mutex.
// ===========================================================================
struct SharedData
{
  float    temperature;   // °C simulée
  int      soc;           // State of Charge simulé (%)
  bool     ledState;      // état de la LED (piloté par bouton)
  uint32_t lastUpdate;    // timestamp de la dernière mise à jour capteur
};

SharedData      data;
SemaphoreHandle_t mutexData;

// --- Objets réseau ---
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===========================================================================
// Tâche Sensors : simule la lecture de capteurs et met à jour SharedData
// Dans le vrai projet : lecture CAN bus, Modbus, ADC...
// ===========================================================================
void tacheSensors(void *param)
{
  esp_task_wdt_add(NULL); // Enregistre cette tâche auprès du watchdog

  int soc = 50;
  float temp = 25.0;

  while (true)
  {
    esp_task_wdt_reset(); // Nourrit le watchdog — prouve que la tâche est vivante

    // Simulation de variation capteurs
    temp += (random(-10, 11)) / 10.0;
    temp  = constrain(temp, 20.0, 45.0);
    soc  += random(-1, 2);
    soc   = constrain(soc, 0, 100);

    // Écriture dans SharedData → TOUJOURS sous mutex
    if (xSemaphoreTake(mutexData, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      data.temperature = temp;
      data.soc         = soc;
      data.lastUpdate  = millis();
      xSemaphoreGive(mutexData);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ===========================================================================
// Tâche Display : lit SharedData et rafraîchit le LCD
// Dans le vrai projet : DisplayManager avec U8g2
// ===========================================================================
void tacheDisplay(void *param)
{
  esp_task_wdt_add(NULL);

  while (true)
  {
    esp_task_wdt_reset();

    // Lecture de SharedData → TOUJOURS sous mutex
    float temp;
    int   soc;
    bool  led;
    if (xSemaphoreTake(mutexData, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      temp = data.temperature;
      soc  = data.soc;
      led  = data.ledState;
      xSemaphoreGive(mutexData);
    }

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C SOC:");
    lcd.print(soc);
    lcd.print("%  ");

    lcd.setCursor(0, 1);
    lcd.print("LED:");
    lcd.print(led ? "ON " : "OFF");

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// ===========================================================================
// Tâche MQTT : lit SharedData et publie les données
// Dans le vrai projet : MqttLogger avec reconnexion automatique
// ===========================================================================
void tacheMQTT(void *param)
{
  esp_task_wdt_add(NULL);

  mqtt.setServer(mqttBroker, 1883);

  while (true)
  {
    esp_task_wdt_reset();

    if (!mqtt.connected())
    {
      if (mqtt.connect("ESP32_freertos_step6"))
        Serial.println("MQTT connecté");
    }

    if (mqtt.connected())
    {
      float temp;
      int   soc;

      if (xSemaphoreTake(mutexData, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        temp = data.temperature;
        soc  = data.soc;
        xSemaphoreGive(mutexData);
      }

      mqtt.publish("device/temperature", String(temp, 1).c_str());
      mqtt.publish("device/soc",         String(soc).c_str());
      mqtt.loop();
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// ===========================================================================
// Tâche Bouton : détecte les appuis et met à jour SharedData via queue
// Utilise une queue comme au step-4 — pas d'accès direct à SharedData
// depuis l'ISR ou la tâche bouton pour garder la séparation propre
// ===========================================================================
QueueHandle_t queueBouton;

void tacheBouton(void *param)
{
  esp_task_wdt_add(NULL);
  pinMode(BTN_GRN, INPUT_PULLUP);
  pinMode(LED_GRN, OUTPUT);

  bool prev = HIGH;
  bool msg  = true;

  while (true)
  {
    esp_task_wdt_reset();

    bool cur = digitalRead(BTN_GRN);
    if (prev == HIGH && cur == LOW)
      xQueueSend(queueBouton, &msg, 0);

    prev = cur;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// Tâche qui consomme la queue bouton et met à jour SharedData + LED physique
void tacheLed(void *param)
{
  esp_task_wdt_add(NULL);

  bool msg;
  while (true)
  {
    esp_task_wdt_reset();

    if (xQueueReceive(queueBouton, &msg, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      if (xSemaphoreTake(mutexData, pdMS_TO_TICKS(100)) == pdTRUE)
      {
        data.ledState = !data.ledState;
        digitalWrite(LED_GRN, data.ledState);
        xSemaphoreGive(mutexData);
      }
    }
    else
      esp_task_wdt_reset(); // Nourrit quand même si pas de message
  }
}

// ===========================================================================
// setup()
// ===========================================================================
void setup()
{
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.print("Demarrage...");

  // WiFi — bloquant uniquement au démarrage, acceptable dans setup()
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi connecté");

  // Watchdog global
  esp_task_wdt_config_t wdt = {
    .timeout_ms     = WDT_TIMEOUT_S * 1000,
    .idle_core_mask = 0,
    .trigger_panic  = false
  };
  esp_task_wdt_reconfigure(&wdt);

  // Initialisation des primitives FreeRTOS
  mutexData   = xSemaphoreCreateMutex();
  queueBouton = xQueueCreate(5, sizeof(bool));

  // Initialisation de SharedData
  data = {25.0, 50, false, 0};

  // Création des tâches — réparties sur les deux cœurs
  //                              nom              stack   param prio  handle core
  xTaskCreatePinnedToCore(tacheSensors, "Sensors", 2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(tacheDisplay, "Display", 3072, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(tacheMQTT,   "MQTT",    4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tacheBouton, "Bouton",  2048, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(tacheLed,    "LED",     2048, NULL, 2, NULL, 0);

  Serial.println("Toutes les tâches démarrées.");
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
}
