#pragma once

// ---------------------------------------------------------------------------
// config.h — Configuration matérielle du projet
//
// Centralise tous les GPIO dans un seul fichier.
// Si tu changes de câblage, tu modifies uniquement ici.
//
// #pragma once : empêche ce fichier d'être inclus deux fois
// si plusieurs .cpp l'incluent.
// ---------------------------------------------------------------------------

// --- GPIO Boutons ---
#define BTN_NEXT 19  // BTN JAUNE → page suivante
#define BTN_PREV 18  // BTN ROUGE → page précédente
