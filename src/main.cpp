// Step-2 : OTA robuste — mode Point d'Accès + mot de passe + timeout
//
// Différence clé avec le step-1 :
//   Step-1 → l'ESP32 se connecte à ton routeur WiFi pour recevoir l'OTA
//   Step-2 → l'ESP32 crée son PROPRE réseau WiFi (mode AP)
//            Tu t'y connectes depuis ton PC, puis tu uploades
//
// Pourquoi le mode AP ?
//   → Indépendant du routeur (fonctionne sur site sans infrastructure WiFi)
//   → Plus sécurisé : le réseau OTA n'existe que pendant la mise à jour
//   → C'est le pattern utilisé sur les vrais projets embarqués
//
// Pour uploader en OTA depuis PlatformIO :
//   1. Appuie sur le bouton vert pour activer le mode OTA
//   2. Connecte ton PC au réseau "esp32-ota" (mot de passe : update123)
//   3. Dans platformio.ini, décommente les 3 lignes OTA
//   4. Clique sur Upload

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <LiquidCrystal_I2C.h>

// --- Version du firmware ---
// Modifie cette valeur et uploade en OTA pour valider que ça fonctionne
#define VERSION "2.0"

// --- Bouton pour activer le mode OTA ---
#define BTN_GRN 16

// --- Réseau AP créé par l'ESP32 ---
const char *AP_SSID     = "esp32-ota";
const char *AP_PASSWORD = "update123";

// --- Mot de passe OTA ---
// Sans ce mot de passe, PlatformIO refusera d'uploader
// Dans platformio.ini, décommente : upload_flags = --auth=update123
const char *OTA_PASSWORD = "update123";

// --- Timeout OTA ---
// Le serveur s'arrête automatiquement après 10 min sans upload
#define OTA_TIMEOUT_MS 600000UL

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool          otaActive     = false;
bool          otaInProgress = false;
unsigned long otaStartTime  = 0;
bool          btnPrev       = HIGH;

// --- Prototypes (nécessaires car les fonctions s'appellent mutuellement) ---
void startOTA();
void stopOTA();
void showVersion();

// ---------------------------------------------------------------------------
// showVersion() : écran principal
// ---------------------------------------------------------------------------
void showVersion()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Version : " VERSION);
  lcd.setCursor(0, 1);
  lcd.print("BTN = mode OTA");
}

// ---------------------------------------------------------------------------
// stopOTA() : arrête le serveur OTA et éteint le point d'accès
// ---------------------------------------------------------------------------
void stopOTA()
{
  ArduinoOTA.end();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  otaActive     = false;
  otaInProgress = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("OTA desactive");
  delay(2000);
  showVersion();
}

// ---------------------------------------------------------------------------
// startOTA() : crée le point d'accès WiFi et démarre le serveur OTA
// ---------------------------------------------------------------------------
void startOTA()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Demarrage OTA...");

  // Passe en mode AP (Access Point) : l'ESP32 devient lui-même un routeur WiFi
  // Il n'a plus besoin de ton routeur pour fonctionner
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  ArduinoOTA.setHostname("esp32-ota");
  ArduinoOTA.setPassword(OTA_PASSWORD); // Protège l'upload par mot de passe

  ArduinoOTA.onStart([]()
  {
    otaInProgress = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mise a jour...");
    lcd.setCursor(0, 1);
    lcd.print("Ne pas eteindre!");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    // Affiche tous les 10% pour ne pas surcharger le LCD
    int pct = progress / (total / 100);
    if (pct % 10 == 0)
    {
      lcd.setCursor(0, 1);
      lcd.print("Prog: ");
      lcd.print(pct);
      lcd.print("%   ");
    }
  });

  ArduinoOTA.onEnd([]()
  {
    otaInProgress = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Termine !");
    lcd.setCursor(0, 1);
    lcd.print("Redemarrage...");
    // L'ESP32 redémarre automatiquement juste après
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    otaInProgress = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erreur OTA");
    lcd.setCursor(0, 1);
    lcd.print("Code: ");
    lcd.print((int)error); // Affiche le code pour aider au debug
    delay(3000);
    stopOTA();
  });

  ArduinoOTA.begin();
  otaActive    = true;
  otaStartTime = millis();

  // Affiche le SSID et l'IP — l'apprenti sait où se connecter
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(AP_SSID);
  lcd.setCursor(0, 1);
  lcd.print(WiFi.softAPIP()); // IP par défaut du mode AP : 192.168.4.1

  Serial.println("Mode OTA actif — SSID : " + String(AP_SSID));
  Serial.println("IP : " + WiFi.softAPIP().toString());
  Serial.println("Timeout dans 10 minutes");
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
  showVersion();

  Serial.println("Firmware v" VERSION " démarré");
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
  bool btnCurrent = digitalRead(BTN_GRN);

  // Front descendant : bouton pressé → active/désactive le mode OTA
  if (btnPrev == HIGH && btnCurrent == LOW)
  {
    if (!otaActive) startOTA();
    else            stopOTA();
    delay(50); // anti-rebond
  }
  btnPrev = btnCurrent;

  if (otaActive)
  {
    // Écoute les connexions OTA entrantes — doit tourner en permanence
    ArduinoOTA.handle();

    // Timeout : arrêt automatique si personne n'a uploadé
    if (!otaInProgress && (millis() - otaStartTime) > OTA_TIMEOUT_MS)
    {
      Serial.println("Timeout OTA — arrêt automatique");
      stopOTA();
    }
  }
}
