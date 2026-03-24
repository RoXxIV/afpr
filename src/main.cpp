// Commandes MQTT depuis le PC :
//   Démarrer le test  : mosquitto_pub -h 192.168.1.80 -t "test/start"  -m "on"
//   Pause / reprise   : mosquitto_pub -h 192.168.1.80 -t "test/pause"  -m "on"
//   Alarme / reprise  : mosquitto_pub -h 192.168.1.80 -t "test/alarme" -m "on"
//   Écouter l'état    : mosquitto_sub -h 192.168.1.80 -t "test/etat" -v

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <xht11.h>

// --- WiFi & MQTT ---
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";
const char *mqttBroker = "192.168.1.X";
const int mqttPort = 1883;

// --- GPIO ---
#define BTN_GRN 16
#define BTN_YLW 19
#define BTN_RED 18
#define LED_YLW 4
#define LED_GRN 2
#define LED_BLU 13
#define BUZZER 5

#define CYCLE_DURATION 10000UL

// --- Périphériques ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
xht11 xht(26);
unsigned char dat[4] = {0, 0, 0, 0};

// --- Machine d'états ---
enum State
{
    IDLE,
    CHARGE,
    DISCHARGE,
    CHARGE_FINAL,
    PAUSED,
    ALARM,
    DONE
};

State state = IDLE;
State prevState = IDLE;

unsigned long cycleStart = 0;
unsigned long pausedAt = 0;

bool btnGrnPrev = HIGH;
bool btnYlwPrev = HIGH;
bool btnRedPrev = HIGH;

// --- WiFi & MQTT ---
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastMqttRetry = 0;
#define MQTT_RETRY_MS 5000

// ---------------------------------------------------------------------------
// publishEtat() : publie l'état courant sur test/etat
// Permet de monitorer le test depuis le PC avec mosquitto_sub
// ---------------------------------------------------------------------------
void publishEtat(const char *etat)
{
    mqtt.publish("test/etat", etat);
}

// ---------------------------------------------------------------------------
// Utilitaires hardware (inchangés du projet-1)
// ---------------------------------------------------------------------------
void setLeds(bool ylw, bool grn, bool blu)
{
    digitalWrite(LED_YLW, ylw);
    digitalWrite(LED_GRN, grn);
    digitalWrite(LED_BLU, blu);
}

void updateDisplay(int soc, const char *label)
{
    xht.receive(dat);

    lcd.setCursor(0, 0);
    lcd.print("SOC:");
    lcd.print(soc);
    lcd.print("%  T:");
    lcd.print(dat[2]);
    lcd.print("C   ");

    lcd.setCursor(0, 1);
    lcd.print(label);
    lcd.print("              ");
}

// ---------------------------------------------------------------------------
// doStart() / doPause() / doAlarme() : actions centralisées
// Appelées indifféremment par les boutons physiques OU par le callback MQTT
// → un seul endroit pour la logique, deux façons de déclencher
// ---------------------------------------------------------------------------
void doStart(unsigned long now)
{
    if (state != IDLE)
        return;
    state = CHARGE;
    cycleStart = now;
    publishEtat("CHARGE");
}

void doPause(unsigned long now)
{
    if (state != IDLE && state != PAUSED && state != ALARM && state != DONE)
    {
        prevState = state;
        state = PAUSED;
        pausedAt = now;
        setLeds(false, false, false);
        publishEtat("PAUSED");
    }
    else if (state == PAUSED)
    {
        cycleStart += now - pausedAt;
        state = prevState;
        publishEtat("REPRISE");
    }
}

void doAlarme(unsigned long now)
{
    if (state != IDLE && state != ALARM && state != DONE)
    {
        prevState = state;
        state = ALARM;
        pausedAt = now;
        setLeds(false, false, false);
        digitalWrite(BUZZER, HIGH);
        publishEtat("ALARM");
    }
    else if (state == ALARM)
    {
        cycleStart += now - pausedAt;
        digitalWrite(BUZZER, LOW);
        state = prevState;
        publishEtat("REPRISE");
    }
}

// ---------------------------------------------------------------------------
// callback() : messages MQTT entrants depuis le PC
// ---------------------------------------------------------------------------
void callback(char *topic, byte *payload, unsigned int length)
{
    unsigned long now = millis();

    String t = String(topic);
    if (t == "test/start")
        doStart(now);
    else if (t == "test/pause")
        doPause(now);
    else if (t == "test/alarme")
        doAlarme(now);
}

// ---------------------------------------------------------------------------
// connectMQTT() : reconnexion non-bloquante
// ---------------------------------------------------------------------------
void connectMQTT(unsigned long now)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        return;
    }

    if (now - lastMqttRetry < MQTT_RETRY_MS)
        return;
    lastMqttRetry = now;

    Serial.print("Connexion MQTT...");
    if (mqtt.connect("ESP32_projet2"))
    {
        Serial.println(" connecté !");
        mqtt.subscribe("test/start");
        mqtt.subscribe("test/pause");
        mqtt.subscribe("test/alarme");
        publishEtat("IDLE");
    }
    else
    {
        Serial.print(" échec, code=");
        Serial.println(mqtt.state());
    }
}

// ---------------------------------------------------------------------------
// safetyWatch() : boutons pause et alarme (inchangé, délégue aux fonctions)
// ---------------------------------------------------------------------------
void safetyWatch(unsigned long now)
{
    bool btnYlw = digitalRead(BTN_YLW);
    bool btnRed = digitalRead(BTN_RED);

    if (btnYlwPrev == HIGH && btnYlw == LOW)
    {
        doPause(now);
        delay(50);
    }
    if (btnRedPrev == HIGH && btnRed == LOW)
    {
        doAlarme(now);
        delay(50);
    }

    btnYlwPrev = btnYlw;
    btnRedPrev = btnRed;
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    pinMode(BTN_GRN, INPUT_PULLUP);
    pinMode(BTN_YLW, INPUT_PULLUP);
    pinMode(BTN_RED, INPUT_PULLUP);
    pinMode(LED_YLW, OUTPUT);
    pinMode(LED_GRN, OUTPUT);
    pinMode(LED_BLU, OUTPUT);
    pinMode(BUZZER, OUTPUT);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Test batterie");
    lcd.setCursor(0, 1);
    lcd.print("Connexion WiFi");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
        delay(500);

    lcd.setCursor(0, 1);
    lcd.print("Appui btn vert ");

    mqtt.setServer(mqttBroker, mqttPort);
    mqtt.setCallback(callback);
    mqtt.setKeepAlive(60);
    connectMQTT(0);
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

    safetyWatch(now);

    bool btnGrn = digitalRead(BTN_GRN);

    switch (state)
    {
    case IDLE:
        if (btnGrnPrev == HIGH && btnGrn == LOW)
        {
            doStart(now);
            delay(50);
        }
        break;

    case CHARGE:
        setLeds(true, false, false);
        {
            int soc = (int)((now - cycleStart) * 100 / CYCLE_DURATION);
            if (soc > 100)
                soc = 100;
            updateDisplay(soc, "Charge");
        }
        if (now - cycleStart >= CYCLE_DURATION)
        {
            state = DISCHARGE;
            cycleStart = now;
            publishEtat("DISCHARGE");
        }
        break;

    case DISCHARGE:
        setLeds(false, true, false);
        {
            int soc = 100 - (int)((now - cycleStart) * 100 / CYCLE_DURATION);
            if (soc < 0)
                soc = 0;
            updateDisplay(soc, "Decharge");
        }
        if (now - cycleStart >= CYCLE_DURATION)
        {
            state = CHARGE_FINAL;
            cycleStart = now;
            publishEtat("CHARGE_FINAL");
        }
        break;

    case CHARGE_FINAL:
        setLeds(false, false, true);
        {
            int soc = (int)((now - cycleStart) * 100 / CYCLE_DURATION);
            if (soc > 100)
                soc = 100;
            updateDisplay(soc, "Charge finale");
        }
        if (now - cycleStart >= CYCLE_DURATION)
        {
            state = DONE;
            publishEtat("DONE");
        }
        break;

    case PAUSED:
    case ALARM:
        break;

    case DONE:
        setLeds(true, true, true);
        updateDisplay(100, "Test termine!");
        break;
    }

    btnGrnPrev = btnGrn;
}
