#pragma once

#include <Arduino.h>
#include <DisplayManager.h>

// MenuManager : gestion des pages et de la navigation
//
// Une "page" est une fonction qui sait comment s'afficher.
// MenuManager garde la liste des pages et sait quelle page est active.
//
// Navigation :
//   next()  → page suivante (boucle sur la première après la dernière)
//   prev()  → page précédente
//   refresh() → redessine la page courante (à appeler dans loop)

// Type d'une fonction de page : prend un DisplayManager& et des données
// Le void* permet de passer n'importe quelle structure de données à la page
typedef void (*PageFunc)(DisplayManager &display, void *data);

#define MAX_PAGES 8

class MenuManager
{
public:
  MenuManager(DisplayManager &display);

  // Ajoute une page — dans l'ordre d'ajout
  void addPage(PageFunc page, void *data = nullptr);

  void next();
  void prev();

  // Redessine la page courante — à appeler régulièrement dans loop()
  void refresh();

  // Force le redessinage même si la page n'a pas changé
  // Utile quand les données affichées ont été mises à jour
  void markDirty();

  int currentPage();

private:
  DisplayManager &_display;
  PageFunc        _pages[MAX_PAGES];
  void           *_data[MAX_PAGES];
  int             _count;
  int             _current;
  bool            _needsRedraw;
};
