// Module 5 - Step 7 : NVS — Stockage persistant
//
// Jusqu'ici toutes tes variables sont en RAM → perdues à chaque reboot.
// Le NVS (Non-Volatile Storage) est une zone de la flash de l'ESP32
// qui SURVIT aux redémarrages, coupures de courant, et mises à jour OTA.
//
// C'est ce qu'on utilise dans le vrai projet pour stocker la config des
// bancs (seuils, calibration...) — sans NVS, tout serait perdu à chaque
// coupure de courant sur le terrain.
//
// L'API "Preferences" est le wrapper Arduino autour du NVS brut de l'ESP-IDF.
// Elle organise les données en "namespaces" (comme des tiroirs) et en clés/valeurs.
//
// Ce step démontre la persistance EN DEUX TEMPS :
//   1. Observe le compteur de démarrages s'incrémenter à chaque reboot
//   2. Maintiens le bouton rouge au démarrage → factory reset (remet à zéro)

#include <Arduino.h>
#include <Preferences.h>  // Wrapper Arduino pour le NVS — intégré au framework ESP32

#define BTN_RED 18

Preferences prefs;

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  pinMode(BTN_RED, INPUT_PULLUP);

  // open() ouvre un namespace NVS — comme un tiroir nommé
  // Paramètres :
  //   nom du namespace → chaîne courte, max 15 caractères
  //   readOnly         → false = lecture + écriture
  //
  // Tous les get/put qui suivent opèrent dans ce namespace
  prefs.begin("config", false);

  // --- Factory reset ---
  // Si le bouton rouge est maintenu AU DÉMARRAGE → efface tout le namespace
  // Pattern classique sur les devices terrain pour récupérer une config corrompue
  if (digitalRead(BTN_RED) == LOW)
  {
    prefs.clear(); // Efface toutes les clés du namespace "config"
    Serial.println(">>> FACTORY RESET — toutes les valeurs effacées <<<");
  }

  // --- Compteur de démarrages ---
  // getInt("clé", valeur_par_defaut) :
  //   → si la clé existe dans le NVS : retourne la valeur stockée
  //   → si la clé n'existe pas (premier démarrage) : retourne la valeur par défaut
  int nbDemarrages = prefs.getInt("boots", 0);
  nbDemarrages++;
  prefs.putInt("boots", nbDemarrages); // Sauvegarde immédiatement en flash

  Serial.println("==================================");
  Serial.print("Nombre de démarrages : ");
  Serial.println(nbDemarrages);
  Serial.println("(reboot et observe que le compteur s'incrémente)");
  Serial.println("(maintiens BTN rouge au démarrage pour reset à 0)");

  // --- Config persistante ---
  // Simule le stockage d'une config device — survit aux reboots et aux OTA
  // Dans le vrai projet : seuils de tension, offsets de calibration...
  String nomDevice = prefs.getString("nom", "device_defaut");
  int    seuil     = prefs.getInt("seuil", 80);

  Serial.print("Nom device : ");
  Serial.println(nomDevice);
  Serial.print("Seuil SOC  : ");
  Serial.print(seuil);
  Serial.println("%");
  Serial.println("==================================");

  // Sauvegarde de nouvelles valeurs si c'est le premier démarrage
  // (les putX n'écrasent que si la valeur change — économise les cycles flash)
  if (nomDevice == "device_defaut")
  {
    prefs.putString("nom",   "esp32-prod-01");
    prefs.putInt   ("seuil", 80);
    Serial.println("Config initiale sauvegardée — reboot pour la voir chargée.");
  }

  // end() ferme le namespace et libère les ressources
  // Bonne pratique : toujours fermer après utilisation
  prefs.end();
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
  // NVS n'est pas utilisé en continu — on ouvre, lit/écrit, ferme.
  // Si tu dois mettre à jour une valeur régulièrement (ex: SOC toutes les minutes),
  // ouvre et ferme à chaque fois pour éviter la corruption en cas de coupure.
  delay(1000);
}
