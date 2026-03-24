// lien tuto lié a ce projet :
// https://www.notion.so/MQTT-Installer-Mosquitto-tester-avec-ESP32-32d9107040ad80a19df3e2e85faf478b?source=copy_link
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h> // Bibliothèque MQTT pour Arduino/ESP32

// --- Identifiants WiFi ---
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

// --- Broker MQTT ---
// Adresse IP de ton PC sur le réseau local (où tourne Mosquitto)
// Trouve-la avec "ipconfig" (Windows) ou "ip a" (Linux/Mac)
const char *mqttBroker = "192.168.1.X";
const int mqttPort = 1883; // Port par défaut de Mosquitto (pas de TLS)

// --- LEDs ---
// On garde les mêmes GPIO que les steps précédents pour la cohérence
#define LED_RED 5  // GPIO 5
#define LED_YLW 4  // GPIO 4
#define LED_GRN 2  // GPIO 2
#define LED_BLU 13 // GPIO 13

// --- Objets WiFi et MQTT ---
// WiFiClient : gère la connexion TCP sous-jacente
// PubSubClient s'appuie dessus pour faire du MQTT par-dessus TCP
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// ---------------------------------------------------------------------------
// callback() : appelée automatiquement par PubSubClient à chaque message reçu
//
// Paramètres :
//   topic   : le topic du message reçu  (ex: "led/rouge")
//   payload : le contenu du message     (ex: "on" ou "off")
//             → c'est un tableau d'octets, PAS une String directement
//   length  : nombre d'octets dans payload
// ---------------------------------------------------------------------------
void callback(char *topic, byte *payload, unsigned int length)
{
    // Convertit payload (tableau d'octets) en String lisible
    String message = "";
    for (unsigned int i = 0; i < length; i++)
        message += (char)payload[i];

    // Détermine quelle broche piloter selon le topic reçu
    int pin = -1;
    if (String(topic) == "led/rouge")
        pin = LED_RED;
    else if (String(topic) == "led/jaune")
        pin = LED_YLW;
    else if (String(topic) == "led/vert")
        pin = LED_GRN;
    else if (String(topic) == "led/bleu")
        pin = LED_BLU;

    if (pin == -1)
        return; // Topic inconnu, on ignore

    // Allume ou éteint la LED selon le message
    if (message == "on")
        digitalWrite(pin, HIGH);
    else if (message == "off")
        digitalWrite(pin, LOW);
}

// ---------------------------------------------------------------------------
// connectMQTT() : (re)connexion au broker MQTT + abonnements aux topics
//
// PubSubClient ne gère pas la reconnexion automatique.
// On appelle cette fonction dans loop() si la connexion est perdue.
// ---------------------------------------------------------------------------
void connectMQTT()
{
    while (!mqtt.connected())
    {
        Serial.print("Connexion MQTT...");

        // connect(clientId) : identifiant unique de ce client sur le broker
        // Si deux clients ont le même ID, le broker déconnecte le premier
        if (mqtt.connect("ESP32_step8"))
        {
            Serial.println(" connecté !");

            // subscribe() : s'abonne à un topic
            // Le broker enverra désormais tous les messages de ces topics à notre callback
            mqtt.subscribe("led/rouge");
            mqtt.subscribe("led/jaune");
            mqtt.subscribe("led/vert");
            mqtt.subscribe("led/bleu");
        }
        else
        {
            // mqtt.state() retourne un code d'erreur numérique (voir doc PubSubClient)
            Serial.print(" échec, code=");
            Serial.print(mqtt.state());
            Serial.println(" → nouvel essai dans 2s");
            delay(2000);
        }
    }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    // Configuration des LEDs en sortie
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YLW, OUTPUT);
    pinMode(LED_GRN, OUTPUT);
    pinMode(LED_BLU, OUTPUT);

    // Connexion WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connexion WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("WiFi connecté – IP : ");
    Serial.println(WiFi.localIP());

    // Configure le broker MQTT et la fonction callback
    mqtt.setServer(mqttBroker, mqttPort);
    mqtt.setCallback(callback);

    // Première connexion
    connectMQTT();
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
    // Si la connexion MQTT est perdue, on se reconnecte
    if (!mqtt.connected())
        connectMQTT();

    // mqtt.loop() : traite les messages entrants et maintient la connexion active
    // DOIT être appelé à chaque tour de boucle, sinon le broker déconnecte le client
    mqtt.loop();
}
