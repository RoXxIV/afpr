// Commandes Mosquitto utiles :
//   Allumer une LED    : mosquitto_pub -h 192.168.1.80 -t "led/rouge" -m "on"
//   Allumer tout       : mosquitto_pub -h 192.168.1.80 -t "led/all" -m "on"
//   Écouter toutes LEDs: mosquitto_sub -h 192.168.1.80 -t "led/#" -v

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- Identifiants WiFi ---
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

// --- Broker MQTT ---
const char *mqttBroker = "192.168.1.X";
const int mqttPort = 1883;

// --- LEDs ---
#define LED_RED 5
#define LED_YLW 4
#define LED_GRN 2
#define LED_BLU 13

// --- Boutons ---
#define BTN_RED 18
#define BTN_YLW 19
#define BTN_GRN 16
#define BTN_BLU 17

// --- Topics et broches ---
const char *topics[] = {"led/rouge", "led/jaune", "led/vert", "led/bleu"};
const int ledPins[] = {LED_RED, LED_YLW, LED_GRN, LED_BLU};
const int btnPins[] = {BTN_RED, BTN_YLW, BTN_GRN, BTN_BLU};
const int NB_LEDS = 4;

// Topic spéciaux pour piloter toutes les LEDs d'un coup depuis le PC :
//   mosquitto_pub -h 192.168.1.80 -t "led/all" -m "on"
//   mosquitto_pub -h 192.168.1.80 -t "led/all" -m "off"
#define TOPIC_ALL "led/all"

// --- État et anti-rebond ---
bool ledState[NB_LEDS] = {false, false, false, false};
bool btnPrev[NB_LEDS] = {HIGH, HIGH, HIGH, HIGH};
bool btnHandled[NB_LEDS] = {false, false, false, false};
unsigned long lastDebounce[NB_LEDS] = {0, 0, 0, 0};
#define DEBOUNCE_MS 50

// --- Objets WiFi et MQTT ---
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

#define MQTT_RETRY_MS 5000
// Initialisé à -MQTT_RETRY_MS pour que le premier appel à connectMQTT()
// passe immédiatement le guard "now - lastMqttRetry < MQTT_RETRY_MS"
// Avec unsigned long, (0 - 5000) wrappe à ULONG_MAX - 4999 → valeur très grande
// → now - lastMqttRetry sera toujours >= MQTT_RETRY_MS au premier appel
unsigned long lastMqttRetry = (unsigned long)-MQTT_RETRY_MS;

// ---------------------------------------------------------------------------
// setLed() : applique un état sur une LED
// publish=true  → utilisé par les boutons : met à jour + publie sur MQTT
// publish=false → utilisé par le callback : met à jour seulement, sans
//                 republier, pour éviter la boucle infinie
//                 (l'ESP32 est abonné à ses propres topics → il recevrait
//                  ses propres messages → callback → publish → ...)
// ---------------------------------------------------------------------------
void setLed(int index, bool state, bool publish = true)
{
    ledState[index] = state;
    digitalWrite(ledPins[index], state);
    if (publish)
    {
        mqtt.publish(topics[index], state ? "on" : "off");
        Serial.print("Publié → ");
        Serial.print(topics[index]);
        Serial.println(state ? " : on" : " : off");
    }
}

// ---------------------------------------------------------------------------
// callback() : messages MQTT entrants (depuis le PC)
// ---------------------------------------------------------------------------
void callback(char *topic, byte *payload, unsigned int length)
{
    String message = "";
    for (unsigned int i = 0; i < length; i++)
        message += (char)payload[i];

    // Commande globale : allume ou éteint toutes les LEDs
    if (String(topic) == TOPIC_ALL)
    {
        bool state = (message == "on");
        for (int i = 0; i < NB_LEDS; i++)
            setLed(i, state, false); // false = pas de republication
        return;
    }

    // Commande individuelle par topic
    for (int i = 0; i < NB_LEDS; i++)
    {
        if (String(topic) == topics[i])
        {
            if (message == "on")
                setLed(i, true, false);
            else if (message == "off")
                setLed(i, false, false);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// connectMQTT() : reconnexion non-bloquante
// ---------------------------------------------------------------------------
void connectMQTT(unsigned long now)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi perdu, reconnexion...");
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        return;
    }

    if (now - lastMqttRetry < MQTT_RETRY_MS)
        return;
    lastMqttRetry = now;

    Serial.print("Connexion MQTT...");
    if (mqtt.connect("ESP32_step10"))
    {
        Serial.println(" connecté !");
        for (int i = 0; i < NB_LEDS; i++)
            mqtt.subscribe(topics[i]);
        mqtt.subscribe(TOPIC_ALL); // Abonnement au topic global
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
    // Désactive le mode économie d'énergie WiFi
    // Sans ça, l'ESP32 dort entre les paquets et ne reçoit les messages MQTT
    // qu'aux intervalles DTIM du routeur → latence de plusieurs secondes
    WiFi.setSleep(false);
    Serial.println();
    Serial.print("WiFi connecté – IP : ");
    Serial.println(WiFi.localIP());

    mqtt.setServer(mqttBroker, mqttPort);
    mqtt.setCallback(callback);
    mqtt.setKeepAlive(60);
    connectMQTT(millis()); // Le guard passe immédiatement grâce à l'init de lastMqttRetry
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
    unsigned long now = millis();

    if (!mqtt.connected())
        connectMQTT(now);

    mqtt.loop();

    // --- Lecture des boutons ---
    for (int i = 0; i < NB_LEDS; i++)
    {
        bool btnCurrent = digitalRead(btnPins[i]);

        if (btnPrev[i] == HIGH && btnCurrent == LOW)
        {
            lastDebounce[i] = now;
            btnHandled[i] = false;
        }

        if ((now - lastDebounce[i]) >= DEBOUNCE_MS && btnCurrent == LOW && !btnHandled[i])
        {
            btnHandled[i] = true;
            setLed(i, !ledState[i]); // Toggle + publish
        }

        if (btnPrev[i] == LOW && btnCurrent == HIGH)
            btnHandled[i] = false;

        btnPrev[i] = btnCurrent;
    }
}
