#pragma once
#include <TFT_eSPI.h>
#include "macropad.h"
#include "keys.h"
#include "keymap.h"

// ── Palette — macOS dark UI ───────────────────────────────────
// System Background layers
#define C_BG            0x18C3   // #161618  deepest background
#define C_PANEL         0x2945   // #2c2c2e  elevated surface (cards, bars)
#define C_PANEL_HI      0x39E7   // #3a3a3c  hovered / active surface
#define C_SEPARATOR     0x4228   // #48484a  separator lines, borders

// Text
#define C_TEXT          0xEF7B   // #ebebf5  primary label
#define C_TEXT_SEC      0xAD55   // #ababab  secondary label
#define C_TEXT_TER      0x6B4D   // #6b6b6b  tertiary / hint

// Accent colours (macOS system palette)
#define C_BLUE          0x051F   // #0a84ff  system blue  (active key, BLE dot)
#define C_GREEN         0x2D8B   // #28c840  system green (BLE connected)
#define C_RED           0xFA08   // #ff453a  system red   (mute, BLE off)
#define C_AMBER         0xFF25   // #ffd60a  system yellow (clipboard icons)
#define C_PURPLE        0x981F   // #bf5af2  optional accent

// Active-key tint background
#define C_ACTIVE_BG     0x0E5A   // #1c3a5e  blue wash for pressed/last key

// Traffic-light dots
#define C_TL_RED        0xFA8E   // #ff5f57
#define C_TL_AMBER      0xFF25   // #febc2e
#define C_TL_GREEN      0x2D8B   // #28c840

// Flash / toast
#define C_TOAST_BG      0x2945   // same as C_PANEL — pill matches the bar
#define C_FLASH         0x051F   // blue dot in toast

// ── Layout ───────────────────────────────────────────────────
#define SCREEN_W        240
#define SCREEN_H        240

#define MENUBAR_H       28       // top menu-bar height
#define GRID_TOP        (MENUBAR_H + 2)
#define CELL_W          78       // (240 - 6px gaps) / 3  ≈ 78
#define CELL_H          54
#define CELL_R          9        // corner radius for key cards
#define CELL_GAP        3        // gap between cells

#define GRID_H          (CELL_H * 3 + CELL_GAP * 2)   // 3 rows + 2 gaps = 170
#define DOCK_Y          (GRID_TOP + GRID_H + 3)        // encoder dock top
#define DOCK_H          22
#define DOCK_R          11       // pill radius

#define TOAST_Y         (DOCK_Y + DOCK_H + 3)
#define TOAST_H         16
#define TOAST_R         8

extern TFT_eSPI tft;

// ── Display state ─────────────────────────────────────────────
extern bool     screenDirty;
extern char     lastActionLabel[32];
extern uint32_t lastActionMs;
extern bool     lastActionVisible;
extern uint8_t  lastActionKey;    // 0-8 key index, 0xFF = encoder/other

// ── Public API ────────────────────────────────────────────────
void displayInit();
void displayDraw(uint8_t mode, bool bleConnected);
void displaySetLastAction(const char* label, uint8_t keyIdx = 0xFF);
void displayTick(uint8_t mode, bool bleConnected);
