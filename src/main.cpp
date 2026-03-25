// Step-3 : OTA déclenchée à distance via MQTT
//
// Jusqu'ici (step-2), tu activais l'OTA en appuyant sur un bouton,
// puis tu uploadais depuis PlatformIO.
//
// Ici, c'est différent : l'ESP32 reçoit une commande MQTT depuis le PC
// avec l'URL du firmware à télécharger, et il gère tout seul le téléchargement.
// Tu n'as plus besoin de PlatformIO pour mettre à jour un device déployé.
//
// C'est le pattern utilisé en production pour mettre à jour des devices
// sur le terrain sans y avoir accès physiquement.
//
// Flow complet :
//   1. Compiler dans PlatformIO → génère .pio/build/esp32dev/firmware.bin
//   2. Servir le fichier :  python -m http.server 80 --directory .pio/build/esp32dev/
//   3. Envoyer la commande : mosquitto_pub -h 192.168.1.80 -t "esp32/ota/cmd"
//                            -m '{"version":"3.1","url":"http://192.168.1.80/firmware.bin"}'
//   4. Suivre la progression : mosquitto_sub -h 192.168.1.80 -t "esp32/ota/#" -v

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPUpdate.h> // Télécharge et installe un firmware depuis une URL HTTP
#include <ArduinoOTA.h> // Conservé du step-2 pour l'upload manuel si besoin
#include <PubSubClient.h>
#include <ArduinoJson.h> // Parse le payload JSON de la commande OTA
#include <LiquidCrystal_I2C.h>

// --- Version du firmware ---
// L'ESP32 refuse toute mise à jour vers une version inférieure ou égale
#define VERSION "3.0"
#define VERSION_INT 30 // Version sous forme entière pour la comparaison (3.0 → 30)

// --- WiFi ---
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// --- MQTT ---
const char *mqttBroker = "192.168.1.XX";
const int mqttPort = 1883;

// --- Topics MQTT ---
// Même structure que le vrai projet multi-bat
#define TOPIC_OTA_CMD "esp32/ota/cmd"           // PC → ESP32 : commande de mise à jour
#define TOPIC_OTA_PROGRESS "esp32/ota/progress" // ESP32 → PC : progression en %
#define TOPIC_OTA_RESULT "esp32/ota/result"     // ESP32 → PC : résultat final

// --- Mode AP (conservé du step-2) ---
const char *AP_SSID = "esp32-ota";
const char *AP_PASSWORD = "update123";
const char *OTA_PASSWORD = "update123";
#define OTA_TIMEOUT_MS 600000UL

LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool otaActive = false;
bool otaInProgress = false;
unsigned long otaStartTime = 0;
bool btnPrev = HIGH;
#define BTN_GRN 16
#define MQTT_RETRY_MS 5000UL
unsigned long lastMqttRetry = (unsigned long)-MQTT_RETRY_MS;

void startOTA();
void stopOTA();
void showVersion();

// ---------------------------------------------------------------------------
// versionToInt() : convertit "3.1" → 31 pour comparer les versions
// Permet de refuser un downgrade (ex: 3.0 → 2.9 refusé)
// ---------------------------------------------------------------------------
int versionToInt(const char *v)
{
    int major = 0, minor = 0;
    sscanf(v, "%d.%d", &major, &minor);
    return major * 10 + minor;
}

// ---------------------------------------------------------------------------
// runHttpOTA() : télécharge et installe le firmware depuis une URL HTTP
// C'est HTTPUpdate qui gère le téléchargement, la vérification et l'écriture
// en flash — on lui passe juste l'URL
// ---------------------------------------------------------------------------
void runHttpOTA(const String &url, const String &targetVersion)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DL firmware...");

    // Callback de progression : publié sur MQTT pour suivre depuis le PC
    httpUpdate.onProgress([](int progress, int total)
                          {
    int pct = (progress * 100) / total;
    mqtt.publish(TOPIC_OTA_PROGRESS, String(pct).c_str());

    if (pct % 10 == 0)
    {
      lcd.setCursor(0, 1);
      lcd.print("Prog: ");
      lcd.print(pct);
      lcd.print("%   ");
    } });

    // HTTPUpdate.update() bloque jusqu'à la fin du téléchargement
    // Retourne HTTP_UPDATE_OK, HTTP_UPDATE_FAILED ou HTTP_UPDATE_NO_UPDATES
    t_httpUpdate_return ret = httpUpdate.update(wifiClient, url);

    switch (ret)
    {
    case HTTP_UPDATE_OK:
        // Ne sera jamais affiché : l'ESP32 redémarre automatiquement
        mqtt.publish(TOPIC_OTA_RESULT, ("OK v" + targetVersion).c_str());
        break;

    case HTTP_UPDATE_FAILED:
        mqtt.publish(TOPIC_OTA_RESULT, "FAILED");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Erreur download");
        lcd.setCursor(0, 1);
        lcd.print(httpUpdate.getLastErrorString());
        delay(3000);
        showVersion();
        break;

    case HTTP_UPDATE_NO_UPDATES:
        mqtt.publish(TOPIC_OTA_RESULT, "NO_UPDATE");
        showVersion();
        break;
    }
}

// ---------------------------------------------------------------------------
// callback() : reçoit les commandes MQTT
//
// Payload JSON attendu sur esp32/ota/cmd :
//   { "version": "3.1", "url": "http://192.168.1.80/firmware.bin" }
//
// L'ESP32 vérifie que la version cible est supérieure à la version actuelle
// avant de lancer le téléchargement — c'est l'anti-downgrade
// ---------------------------------------------------------------------------
void callback(char *topic, byte *payload, unsigned int length)
{
    if (String(topic) != TOPIC_OTA_CMD)
        return;

    // Parse le JSON — ArduinoJson gère les erreurs de format
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err)
    {
        mqtt.publish(TOPIC_OTA_RESULT, "ERR_JSON");
        return;
    }

    const char *targetVersion = doc["version"];
    const char *url = doc["url"];

    if (!targetVersion || !url)
    {
        mqtt.publish(TOPIC_OTA_RESULT, "ERR_MISSING_FIELDS");
        return;
    }

    // Anti-downgrade : refuse si la version cible est inférieure ou égale à l'actuelle
    if (versionToInt(targetVersion) <= VERSION_INT)
    {
        mqtt.publish(TOPIC_OTA_RESULT, "ERR_VERSION_TOO_LOW");
        Serial.println("OTA refusée — version cible <= version actuelle");
        return;
    }

    Serial.println("OTA acceptée → téléchargement depuis " + String(url));
    runHttpOTA(String(url), String(targetVersion));
}

// ---------------------------------------------------------------------------
// connectMQTT() : reconnexion non-bloquante
// ---------------------------------------------------------------------------
void connectMQTT(unsigned long now)
{
    if (mqtt.connected())
        return;
    if (now - lastMqttRetry < MQTT_RETRY_MS)
        return;
    lastMqttRetry = now;

    if (mqtt.connect("ESP32_OTA_step3"))
    {
        mqtt.subscribe(TOPIC_OTA_CMD);
        Serial.println("MQTT connecté");
    }
}

// ---------------------------------------------------------------------------
// showVersion()
// ---------------------------------------------------------------------------
void showVersion()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Version : " VERSION);
    lcd.setCursor(0, 1);
    lcd.print("BTN = mode AP");
}

// ---------------------------------------------------------------------------
// startOTA() / stopOTA() : mode AP pour upload manuel (conservé du step-2)
// Utile si le broker MQTT est indisponible
// ---------------------------------------------------------------------------
void stopOTA()
{
    ArduinoOTA.end();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    otaActive = false;
    otaInProgress = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reconnexion...");
    while (WiFi.status() != WL_CONNECTED)
        delay(500);
    WiFi.setSleep(false); // Retour en mode STA → réactiver pour éviter la latence MQTT
    mqtt.setServer(mqttBroker, mqttPort);
    mqtt.setCallback(callback);
    showVersion();
}

void startOTA()
{
    mqtt.disconnect();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Demarrage OTA...");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    ArduinoOTA.setHostname("esp32-ota");
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]()
                       { otaInProgress = true; });
    ArduinoOTA.onEnd([]()
                     { otaInProgress = false; });
    ArduinoOTA.onError([](ota_error_t)
                       { stopOTA(); });

    ArduinoOTA.begin();
    otaActive = true;
    otaStartTime = millis();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(AP_SSID);
    lcd.setCursor(0, 1);
    lcd.print(WiFi.softAPIP());
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    pinMode(BTN_GRN, INPUT_PULLUP);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Connexion WiFi");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
        delay(500);
    WiFi.setSleep(false);
    Serial.println("WiFi connecté – IP : " + WiFi.localIP().toString());

    mqtt.setServer(mqttBroker, mqttPort);
    mqtt.setCallback(callback);
    connectMQTT(millis());

    showVersion();
    Serial.println("Firmware v" VERSION " démarré — en écoute sur " TOPIC_OTA_CMD);
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
    unsigned long now = millis();

    bool btnCurrent = digitalRead(BTN_GRN);
    if (btnPrev == HIGH && btnCurrent == LOW)
    {
        if (!otaActive)
            startOTA();
        else
            stopOTA();
        delay(50);
    }
    btnPrev = btnCurrent;

    if (otaActive)
    {
        ArduinoOTA.handle();
        if (!otaInProgress && (now - otaStartTime) > OTA_TIMEOUT_MS)
            stopOTA();
    }
    else
    {
        connectMQTT(now);
        mqtt.loop();
    }
}
