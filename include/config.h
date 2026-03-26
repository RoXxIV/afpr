#pragma once

// ---------------------------------------------------------------------------
// config.h — Configuration matérielle du projet
//
// Centralise tous les GPIO et constantes dans un seul fichier.
// Quand tu changes de carte ou de câblage, tu modifies uniquement ce fichier.
//
// Convention : on utilise #define et non des variables globales —
// les defines sont résolus à la compilation, pas à l'exécution.
//
// #pragma once : garde-fou qui empêche ce fichier d'être inclus deux fois
// si plusieurs .cpp l'incluent — équivalent du classique #ifndef/#define/#endif
// ---------------------------------------------------------------------------

// --- GPIO Boutons ---
#define BTN_GRN 16
#define BTN_YLW 19

// --- GPIO LEDs ---
#define LED_GRN 2
#define LED_YLW 4
