#pragma once
#include <TFT_eSPI.h>
#include "macropad.h"
#include "keys.h"
#include "keymap.h"

// ── Palette (RGB565) ─────────────────────────────────────────
#define C_BG        0x1082   // near-black
#define C_PANEL     0x2104   // dark grey panels
#define C_BORDER    0x4208   // mid grey borders
#define C_TEXT      0xFFFF   // white
#define C_DIM       0x8410   // grey for empty keys
#define C_ACCENT    0x07FF   // cyan for mode bar
#define C_BLE_ON    0x07E0   // green dot
#define C_BLE_OFF   0xF800   // red dot
#define C_FLASH     0xFD20   // amber for last-action flash

// ── Layout constants ─────────────────────────────────────────
#define SCREEN_W    240
#define SCREEN_H    240

#define TOP_BAR_H   28
#define GRID_Y      (TOP_BAR_H + 2)
#define CELL_W      80
#define CELL_H      56
#define GRID_H      (CELL_H * 3)          // 168
#define ENC_ROW_Y   (GRID_Y + GRID_H + 2) // ~198
#define ENC_ROW_H   22
#define LAST_Y      (ENC_ROW_Y + ENC_ROW_H + 2)
#define LAST_H      (SCREEN_H - LAST_Y)   // ~10 px left

extern TFT_eSPI tft;

// ── State that display.h tracks ──────────────────────────────
extern bool     screenDirty;
extern char     lastActionLabel[32];
extern uint32_t lastActionMs;
extern bool     lastActionVisible;

void displayInit();
void displayDraw(uint8_t mode, bool bleConnected);
void displaySetLastAction(const char* label);
void displayTick(uint8_t mode, bool bleConnected);   // call every loop — handles last-action fade