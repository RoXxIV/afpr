#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- WiFi & MQTT ---
const char *ssid       = "NovaHome";
const char *password   = "nova1234";
const char *mqttBroker = "192.168.1.80";
const int   mqttPort   = 1883;

WiFiClient   espClient;
PubSubClient client(espClient);

#define NB_BANCS    6
#define NB_CELLULES 15
#define NB_NOURRICES 6
#define PUBLISH_INTERVAL_MS 1000UL

// --- Structure BMS ---
struct BatterieSim
{
    float voltage;
    float current;
    int   soc;
    float temperature;
    float dischargedCapacity;
    float dischargedEnergy;
    int   cellVoltages[NB_CELLULES];
    int   nourriceSoc[NB_NOURRICES];
};

BatterieSim bancs[NB_BANCS];

unsigned long lastPublish    = 0;
#define MQTT_RETRY_MS 5000UL
unsigned long lastMqttRetry  = (unsigned long)-MQTT_RETRY_MS;

// ---------------------------------------------------------------------------
// initBancs()
// ---------------------------------------------------------------------------
void initBancs()
{
    for (int i = 0; i < NB_BANCS; i++)
    {
        bancs[i].voltage            = random(480, 520) / 10.0;
        bancs[i].current            = 0;
        bancs[i].soc                = random(50, 90);
        bancs[i].temperature        = random(250, 300) / 10.0;
        bancs[i].dischargedCapacity = random(0, 1000) / 10.0;
        bancs[i].dischargedEnergy   = random(0, 5000) / 10.0;
        for (int j = 0; j < NB_CELLULES;  j++) bancs[i].cellVoltages[j] = random(3100, 3400);
        for (int n = 0; n < NB_NOURRICES; n++) bancs[i].nourriceSoc[n]  = random(50, 90);
    }
}

// ---------------------------------------------------------------------------
// connectMQTT() : non-bloquant — retente toutes les MQTT_RETRY_MS
// ---------------------------------------------------------------------------
void connectMQTT(unsigned long now)
{
    if (client.connected()) return;
    if (now - lastMqttRetry < MQTT_RETRY_MS) return;
    lastMqttRetry = now;

    Serial.print("Connexion MQTT...");
    if (client.connect("ESP32_BMS_Sim"))
        Serial.println(" connecté !");
    else
    {
        Serial.print(" échec, code=");
        Serial.println(client.state());
    }
}

// ---------------------------------------------------------------------------
// updateBanc() : variation douce des paramètres d'un banc
// ---------------------------------------------------------------------------
void updateBanc(int i)
{
    bancs[i].voltage += random(-5, 6) / 100.0;
    bancs[i].voltage  = constrain(bancs[i].voltage, 42.0, 54.5);

    bancs[i].current += random(-10, 11) / 10.0;
    bancs[i].current  = constrain(bancs[i].current, -100.0, 100.0);

    bancs[i].soc += random(-1, 2);
    bancs[i].soc  = constrain(bancs[i].soc, 0, 100);

    bancs[i].temperature += random(-2, 3) / 10.0;
    bancs[i].temperature  = constrain(bancs[i].temperature, 20.0, 45.0);

    bancs[i].dischargedCapacity += random(0, 5) / 10.0;
    bancs[i].dischargedCapacity  = constrain(bancs[i].dischargedCapacity, 0.0, 291.0);

    bancs[i].dischargedEnergy += random(0, 100) / 10.0;
    bancs[i].dischargedEnergy  = constrain(bancs[i].dischargedEnergy, 0.0, 13968.0);

    for (int j = 0; j < NB_CELLULES; j++)
    {
        bancs[i].cellVoltages[j] += random(-5, 6);
        bancs[i].cellVoltages[j]  = constrain(bancs[i].cellVoltages[j], 2800, 3650);
    }

    for (int n = 0; n < NB_NOURRICES; n++)
    {
        bancs[i].nourriceSoc[n] += random(-4, 5);
        bancs[i].nourriceSoc[n]  = constrain(bancs[i].nourriceSoc[n], 25, 38);
    }
}

// ---------------------------------------------------------------------------
// publishBanc() : construit et envoie les messages MQTT d'un banc
// ---------------------------------------------------------------------------
void publishBanc(int i)
{
    // Calcul max/min cellules
    int maxCellV = 0,    minCellV = 5000;
    int maxCellN = 0,    minCellN = 0;
    for (int j = 0; j < NB_CELLULES; j++)
    {
        if (bancs[i].cellVoltages[j] > maxCellV) { maxCellV = bancs[i].cellVoltages[j]; maxCellN = j + 1; }
        if (bancs[i].cellVoltages[j] < minCellV) { minCellV = bancs[i].cellVoltages[j]; minCellN = j + 1; }
    }

    // CSV BMS principal
    String csv = String(bancs[i].voltage, 2)            + "," +
                 String(bancs[i].current, 2)            + "," +
                 String(bancs[i].soc)                   + "," +
                 String(bancs[i].temperature, 2)        + "," +
                 String(maxCellN)                       + "," +
                 String(maxCellV)                       + "," +
                 String(minCellN)                       + "," +
                 String(minCellV)                       + "," +
                 String(bancs[i].dischargedCapacity, 2) + "," +
                 String(bancs[i].dischargedEnergy, 2);
    for (int j = 0; j < NB_CELLULES; j++)
        csv += "," + String(bancs[i].cellVoltages[j]);

    String topic = "banc" + String(i + 1) + "/bms/data";
    client.publish(topic.c_str(), csv.c_str());
    Serial.println(topic + " : " + csv);

    // SOC nourrices — un publish par nourrice
    for (int n = 0; n < NB_NOURRICES; n++)
    {
        String nTopic = "banc" + String(i + 1) + "/n" + String(n + 1);
        client.publish(nTopic.c_str(), String(bancs[i].nourriceSoc[n]).c_str());
    }
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    // randomSeed évite de rejouer la même séquence à chaque reset
    randomSeed(analogRead(0));

    WiFi.begin(ssid, password);
    Serial.print("Connexion WiFi");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    WiFi.setSleep(false);
    Serial.println("\nWiFi connecté – IP : " + WiFi.localIP().toString());

    client.setServer(mqttBroker, mqttPort);
    client.setKeepAlive(60);

    initBancs();
    connectMQTT(millis());
}

// ---------------------------------------------------------------------------
// loop() : non-bloquant, publie tous les PUBLISH_INTERVAL_MS
// ---------------------------------------------------------------------------
void loop()
{
    unsigned long now = millis();

    connectMQTT(now);
    client.loop();

    if (now - lastPublish < PUBLISH_INTERVAL_MS) return;
    lastPublish = now;

    if (!client.connected()) return; // pas connecté, on skip

    for (int i = 0; i < NB_BANCS; i++)
    {
        updateBanc(i);
        publishBanc(i);
    }
}
