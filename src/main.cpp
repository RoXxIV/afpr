#include <Arduino.h>
#include <WiFi.h> // Bibliothèque WiFi intégrée à l'ESP32

// Identifiants du réseau WiFi
// Remplace par ton SSID et mot de passe
const char* ssid     = "TON_SSID";
const char* password = "TON_MOT_DE_PASSE";

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

void loop()
{
  // Rien à faire en boucle, la connexion est gérée par le système
}
