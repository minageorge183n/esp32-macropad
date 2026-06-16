#pragma once
#include <TFT_eSPI.h>

// ── Secret combos ────────────────────────────────────────────
// Hold two keys simultaneously for COMBO_HOLD_MS to launch.
//
//  SNAKE  : keys 0 + 2  (top-left corner + top-right corner)
//  TETRIS : keys 6 + 8  (bot-left corner + bot-right corner)
//
// While a game is running the normal macropad loop is paused.
// Exit game: hold Enc2 SW (mode button) for 1.2 s  — defined in each game.

#define COMBO_HOLD_MS  1500   // ms both keys must be held

// Call once in setup
void gamesInit(TFT_eSPI* tft);

// Call every loop iteration.
// Returns true if a game is currently running (caller should skip normal macropad logic).
bool gamesTick();
