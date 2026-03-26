// Module 5 - Step 9 : Multi-page UI avec navigation
//
// Ce step assemble les 3 modules créés dans lib/ :
//   ButtonManager  → détecte les appuis boutons
//   DisplayManager → pilote le LCD
//   MenuManager    → gère les pages et la navigation
//
// C'est exactement l'architecture du vrai projet :
//   ButtonManager → MenuManager → DisplayManager
//
// Navigation :
//   BTN JAUNE (19) → page suivante
//   BTN ROUGE (18) → page précédente
//
// 3 pages :
//   Page 1 — Accueil   : nom + numéro de page
//   Page 2 — Capteurs  : température et SOC simulés
//   Page 3 — Système   : uptime en secondes

#include <Arduino.h>
#include <ButtonManager.h>
#include <DisplayManager.h>
#include <MenuManager.h>
#include <config.h>

// --- Données partagées entre les pages ---
// Dans le vrai projet c'est SharedData protégé par mutex (step-6)
struct AppData
{
  float temperature;
  int   soc;
};

AppData appData = {24.5, 78};

// --- Objets ---
DisplayManager display(0x27, 16, 2);
MenuManager    menu(display);
ButtonManager  btnNext(BTN_NEXT); // BTN JAUNE → page suivante
ButtonManager  btnPrev(BTN_PREV); // BTN ROUGE → page précédente

// ===========================================================================
// Définition des pages
//
// Chaque page est une fonction qui reçoit le display et ses données.
// Elle est responsable de son propre affichage — MenuManager ne sait pas
// ce qu'elle affiche, il sait juste quand l'appeler.
// ===========================================================================

void pageAccueil(DisplayManager &d, void *data)
{
  d.printScreen("=== Accueil ===", "BTN: nav pages");
}

void pageCapteurs(DisplayManager &d, void *data)
{
  AppData *ad = (AppData *)data; // Cast du void* vers le vrai type

  String l0 = "T: " + String(ad->temperature, 1) + "C";
  String l1 = "SOC: " + String(ad->soc) + "%";
  d.printScreen(l0, l1);
}

void pageSysteme(DisplayManager &d, void *data)
{
  unsigned long uptime = millis() / 1000;
  String l0 = "Uptime (s):";
  String l1 = String(uptime);
  d.printScreen(l0, l1);
}

// ===========================================================================
// setup()
// ===========================================================================
void setup()
{
  Serial.begin(115200);

  display.begin();
  btnNext.begin();
  btnPrev.begin();

  // Enregistrement des pages dans l'ordre d'affichage
  // Le void* permet de passer les données spécifiques à chaque page
  menu.addPage(pageAccueil,  nullptr);  // pas de données nécessaires
  menu.addPage(pageCapteurs, &appData); // passe un pointeur vers appData
  menu.addPage(pageSysteme,  nullptr);

  // Affichage initial
  menu.refresh();

  Serial.println("UI prête — BTN JAUNE: suivant, BTN ROUGE: précédent");
}

// ===========================================================================
// loop()
// ===========================================================================
void loop()
{
  // Lecture des boutons
  if (btnNext.pressed()) menu.next();
  if (btnPrev.pressed()) menu.prev();

  // Simulation de variation des capteurs toutes les 2s
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 2000)
  {
    lastUpdate = millis();
    appData.temperature += random(-5, 6) / 10.0;
    appData.temperature  = constrain(appData.temperature, 20.0, 40.0);
    appData.soc         += random(-1, 2);
    appData.soc          = constrain(appData.soc, 0, 100);

    // Force le redessinage si on est sur la page capteurs
    // → les données ont changé, il faut rafraîchir l'affichage
    if (menu.currentPage() == 1)
      menu.markDirty();
  }

  menu.refresh();
}
