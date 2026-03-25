#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h> // Bibliothèque OTA, intégrée au framework ESP32 → pas besoin de l'installer
#include <LiquidCrystal_I2C.h>

// --- WiFi ---
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "Your_WIFI_password";

// --- Version du firmware ---
// Modifie cette valeur ("1.0" → "1.1" par exemple), compile et uploade en OTA.
// Si le LCD affiche la nouvelle version sans câble USB → l'OTA a fonctionné !
#define VERSION "1.0"

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------------------------------------------------------------------
// updateDisplay() : rafraîchit le LCD avec la version et l'IP
// L'IP est affichée pour que tu saches sur quelle adresse envoyer le firmware
// ---------------------------------------------------------------------------
void updateDisplay()
{
    lcd.setCursor(0, 0);
    lcd.print("Version : ");
    lcd.print(VERSION);
    lcd.print("   ");

    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP()); // Adresse IP à renseigner dans platformio.ini pour l'upload OTA
    lcd.print("        ");
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Connexion WiFi");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
        delay(500);

    Serial.println("WiFi connecté – IP : " + WiFi.localIP().toString());

    // --- OTA (Over The Air) ---
    // L'OTA permet d'envoyer un nouveau firmware à l'ESP32 via le WiFi,
    // sans brancher le câble USB. Utile quand le device est inaccessible physiquement.
    //
    // Pour uploader en OTA depuis PlatformIO, modifie platformio.ini :
    //   upload_protocol = espota
    //   upload_port     = <IP affichée sur le LCD>
    // puis clique sur Upload normalement.

    ArduinoOTA.setHostname("esp32-ota"); // Nom de l'ESP32 visible sur le réseau local

    // Les callbacks sont des fonctions appelées automatiquement par la lib
    // à chaque étape de la mise à jour — ici on s'en sert pour afficher la progression
    ArduinoOTA.onStart([]()
                       {
    // Déclenché au début du transfert du firmware
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mise a jour OTA"); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          {
    // Déclenché régulièrement pendant le transfert — progress/total donne le pourcentage
    int pct = progress / (total / 100);
    lcd.setCursor(0, 1);
    lcd.print("Prog: ");
    lcd.print(pct);
    lcd.print("%   "); });

    ArduinoOTA.onEnd([]()
                     {
    // Déclenché quand le transfert est terminé, juste avant le reboot
    lcd.setCursor(0, 1);
    lcd.print("Termine !       "); });

    ArduinoOTA.onError([](ota_error_t error)
                       {
    // Déclenché si une erreur survient pendant la mise à jour
    lcd.setCursor(0, 1);
    lcd.print("Erreur OTA      "); });

    ArduinoOTA.begin(); // Démarre le service OTA → l'ESP32 écoute les uploads réseau

    updateDisplay();
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
    // ArduinoOTA.handle() vérifie à chaque tour si un upload OTA est en cours.
    // Sans cet appel, l'ESP32 n'écoutera jamais les connexions OTA entrantes.
    // Attention : un long delay() dans loop() peut faire rater un upload OTA.
    ArduinoOTA.handle();
}
