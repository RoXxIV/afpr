#include <Arduino.h>
#include <WiFi.h> // Bibliothèque WiFi intégrée à l'ESP32

// Identifiants du réseau WiFi
// Remplace par ton SSID et mot de passe
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

void setup()
{
  // Initialise le port série pour afficher des messages sur le moniteur série
  // 115200 = vitesse de communication en bauds (bits par seconde)
  Serial.begin(115200);

  // Démarre la connexion WiFi en mode "station" (client, par opposition à point d'accès)
  WiFi.begin(ssid, password);

  Serial.print("Connexion au WiFi");

  // Attend que la connexion soit établie
  // WL_CONNECTED est une constante qui vaut "connecté"
  // On boucle tant que ce n'est pas le cas
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("."); // Affiche un point toutes les 500ms pour montrer que ça travaille
  }

  Serial.println(); // Saut de ligne
  Serial.println("WiFi connecté !");

  // WiFi.localIP() retourne l'adresse IP attribuée par le routeur (DHCP)
  // On la convertit en texte avec .toString() pour l'afficher
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

unsigned long lastPrint = 0; // Timestamp du dernier affichage

void loop()
{
  // Réutilisation du pattern millis() : affiche l'IP toutes les 5 secondes sans bloquer
  unsigned long maintenant = millis();
  if (maintenant - lastPrint >= 5000)
  {
    lastPrint = maintenant;
    Serial.print("Adresse IP : ");
    Serial.println(WiFi.localIP());
  }
}
