// lien tuto lié a ce projet :
// https://www.notion.so/MQTT-Installer-Mosquitto-tester-avec-ESP32-32d9107040ad80a19df3e2e85faf478b?source=copy_link
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- Identifiants WiFi ---
const char *ssid = "YOURESSID";
const char *password = "YOURPASSWORD";

// --- Broker MQTT ---
const char *mqttBroker = "192.168.1.X";
const int mqttPort = 1883;

// --- LEDs ---
#define LED_RED 5
#define LED_YLW 4
#define LED_GRN 2
#define LED_BLU 13

// --- Boutons (même couleur que la LED associée) ---
#define BTN_RED 18
#define BTN_YLW 19
#define BTN_GRN 16
#define BTN_BLU 17

// --- Topics MQTT ---
// Un tableau parallèle aux broches pour éviter la répétition
const char *topics[] = {"led/rouge", "led/jaune", "led/vert", "led/bleu"};
const int ledPins[] = {LED_RED, LED_YLW, LED_GRN, LED_BLU};
const int btnPins[] = {BTN_RED, BTN_YLW, BTN_GRN, BTN_BLU};
const int NB_LEDS = 4;

// --- État des LEDs et anti-rebond ---
bool ledState[NB_LEDS] = {false, false, false, false};
bool btnPrev[NB_LEDS] = {HIGH, HIGH, HIGH, HIGH};
bool btnHandled[NB_LEDS] = {false, false, false, false}; // vrai si l'appui a déjà été traité
unsigned long lastDebounce[NB_LEDS] = {0, 0, 0, 0};
#define DEBOUNCE_MS 50

// --- Objets WiFi et MQTT ---
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// Reconnexion non-bloquante : timestamp du dernier essai
#define MQTT_RETRY_MS 5000 // essai toutes les 5s max
unsigned long lastMqttRetry = (unsigned long)-MQTT_RETRY_MS;

// ---------------------------------------------------------------------------
// publishLed() : publie l'état d'une LED sur son topic MQTT
// ---------------------------------------------------------------------------
void publishLed(int index)
{
    const char *msg = ledState[index] ? "on" : "off";
    mqtt.publish(topics[index], msg);
    Serial.print("Publié → ");
    Serial.print(topics[index]);
    Serial.print(" : ");
    Serial.println(msg);
}

// ---------------------------------------------------------------------------
// callback() : reçoit les messages MQTT entrants (depuis le PC ou autre)
//
// → Permet de voir côté PC ce que l'ESP32 publie avec :
//     mosquitto_sub -h <IP> -t "led/#"
// → Permet aussi de piloter les LEDs depuis le PC (comme au step-8)
// ---------------------------------------------------------------------------
void callback(char *topic, byte *payload, unsigned int length)
{
    String message = "";
    for (unsigned int i = 0; i < length; i++)
        message += (char)payload[i];

    for (int i = 0; i < NB_LEDS; i++)
    {
        if (String(topic) == topics[i])
        {
            if (message == "on")
            {
                ledState[i] = true;
                digitalWrite(ledPins[i], HIGH);
            }
            else if (message == "off")
            {
                ledState[i] = false;
                digitalWrite(ledPins[i], LOW);
            }
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// connectMQTT() : tentative de (re)connexion non-bloquante
//
// Contrairement à un while+delay, cette fonction fait UN seul essai et rend
// la main immédiatement → mqtt.loop() continue de tourner entre les essais
// → le broker ne nous déconnecte pas pendant la phase de reconnexion
// ---------------------------------------------------------------------------
void connectMQTT(unsigned long now)
{
    // WiFi perdu → on tente de reconnecter le WiFi d'abord
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi perdu, reconnexion...");
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        return; // On réessaiera au prochain tour de loop()
    }

    // Limite le rythme des essais MQTT sans bloquer
    if (now - lastMqttRetry < MQTT_RETRY_MS)
        return;
    lastMqttRetry = now;

    Serial.print("Connexion MQTT...");
    if (mqtt.connect("ESP32_step9"))
    {
        Serial.println(" connecté !");
        for (int i = 0; i < NB_LEDS; i++)
            mqtt.subscribe(topics[i]);
    }
    else
    {
        Serial.print(" échec, code=");
        Serial.print(mqtt.state());
        Serial.println(" → nouvel essai dans 5s");
    }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    for (int i = 0; i < NB_LEDS; i++)
    {
        pinMode(ledPins[i], OUTPUT);
        pinMode(btnPins[i], INPUT_PULLUP);
    }

    WiFi.begin(ssid, password);
    Serial.print("Connexion WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    WiFi.setSleep(false);
    Serial.println();
    Serial.print("WiFi connecté – IP : ");
    Serial.println(WiFi.localIP());

    mqtt.setServer(mqttBroker, mqttPort);
    mqtt.setCallback(callback);
    mqtt.setKeepAlive(60); // Délai keepalive : 60s (défaut 15s, trop court sur réseau instable)
    connectMQTT(millis());
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
    unsigned long now = millis();

    if (!mqtt.connected())
        connectMQTT(now);

    mqtt.loop(); // Traite les messages entrants

    // --- Lecture des boutons ---

    for (int i = 0; i < NB_LEDS; i++)
    {
        bool btnCurrent = digitalRead(btnPins[i]);

        // Front descendant : bouton vient d'être pressé → démarre le chronomètre
        if (btnPrev[i] == HIGH && btnCurrent == LOW)
        {
            lastDebounce[i] = now;
            btnHandled[i] = false; // Nouvel appui, pas encore traité
        }

        // Signal stable depuis DEBOUNCE_MS et appui pas encore traité → action
        if ((now - lastDebounce[i]) >= DEBOUNCE_MS && btnCurrent == LOW && !btnHandled[i])
        {
            btnHandled[i] = true; // Marque comme traité pour ne pas boucler
            ledState[i] = !ledState[i];
            digitalWrite(ledPins[i], ledState[i]);
            publishLed(i);
        }

        // Front montant : bouton relâché → prêt pour un prochain appui
        if (btnPrev[i] == LOW && btnCurrent == HIGH)
            btnHandled[i] = false;

        btnPrev[i] = btnCurrent;
    }
}
