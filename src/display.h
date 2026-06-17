#pragma once
#include <TFT_eSPI.h>
#include "macropad.h"
#include "keys.h"
#include "keymap.h"

// ── Palette (RGB565) — sleek minimal ─────────────────────────
// Near-black background, single cyan accent, everything else neutral
#define C_BG        0x0861   // deep charcoal (not pure black — avoids burn feel)
#define C_PANEL     0x1082   // raised surface — subtle lift from BG
#define C_BORDER    0x2104   // quiet border — present but not loud
#define C_BORDER_HI 0x4208   // slightly brighter border for active cells
#define C_TEXT      0xEF7D   // warm off-white (softer than pure 0xFFFF)
#define C_DIM       0x4A69   // muted grey for empty/inactive elements
#define C_ACCENT    0x05DF   // tighter cyan — less saturated than 0x07FF
#define C_ACCENT_DIM 0x0299  // accent at 40% for BLE pill background
#define C_BLE_ON    0x0400   // deep green dot — not neon
#define C_BLE_OFF   0x6000   // muted red — not alarming
#define C_FLASH     0xFCA0   // warm amber for last-action (softer than 0xFD20)
#define C_STRIPE    0x05DF   // left accent stripe on last-action bar

// ── Layout constants ─────────────────────────────────────────
#define SCREEN_W    240
#define SCREEN_H    240

// Top bar: thinner, cleaner
#define TOP_BAR_H   24

// 1px gap between regions
#define GRID_Y      (TOP_BAR_H + 2)
#define CELL_W      80
#define CELL_H      54
#define GRID_H      (CELL_H * 3)           // 162
#define ENC_ROW_Y   (GRID_Y + GRID_H + 2)  // ~190
#define ENC_ROW_H   24
#define LAST_Y      (ENC_ROW_Y + ENC_ROW_H + 2)
#define LAST_H      (SCREEN_H - LAST_Y)    // ~14 px

// Left accent stripe on last-action bar
#define STRIPE_W    3

extern TFT_eSPI tft;

// ── State that display tracks ─────────────────────────────────
extern bool     screenDirty;
extern char     lastActionLabel[32];
extern uint32_t lastActionMs;
extern bool     lastActionVisible;

void displayInit();
void displayDraw(uint8_t mode, bool bleConnected);
void displaySetLastAction(const char* label);
void displayTick(uint8_t mode, bool bleConnected);