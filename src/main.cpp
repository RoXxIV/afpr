#include <Arduino.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h> // Bibliothèque pour écran LCD via protocole I2C

const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

// Initialise l'écran LCD
// Paramètres : adresse I2C (0x27), nombre de colonnes (16), nombre de lignes (2)
// I2C = protocole de communication sur 2 fils (SDA + SCL) qui permet
// de connecter plusieurs périphériques sur le même bus avec une adresse unique
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup()
{
  Serial.begin(115200);

  // Initialise l'écran et active le rétroéclairage
  lcd.init();
  lcd.backlight();

  // Message d'attente sur le LCD pendant la connexion
  // setCursor(colonne, ligne) : positionne le curseur d'écriture
  // colonne et ligne commencent à 0
  lcd.setCursor(0, 0);
  lcd.print("Connexion WiFi"); // lcd.print() fonctionne comme Serial.print()

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connecté !");
  Serial.println(WiFi.localIP());

  // Connexion établie : affiche l'IP sur le LCD
  // clear() efface tout l'écran avant d'écrire
  lcd.clear();
  lcd.setCursor(0, 0); // Ligne 0 : label
  lcd.print("Adresse IP :");
  lcd.setCursor(0, 1);       // Ligne 1 : valeur de l'IP
  lcd.print(WiFi.localIP()); // LiquidCrystal_I2C sait afficher un objet IPAddress directement
}

void loop()
{
  // L'IP est statique une fois connecté, pas besoin de rafraîchir
}
