#pragma once

// ── Key pins (9 switches) ────────────────────────────────────
//   Pin 15 moved from KEY_PINS[8] → 21 (15 is now TFT_CS)
//   Pin  2 moved from ENC2_SW    → 22 (2  is now TFT_DC)
//   Pin  4 was MODE_PIN          → now TFT_RST (mode = Enc2 SW click)
const int KEY_PINS[9] = {13, 12, 14, 27, 26, 25, 33, 32, 21};

#define NUM_KEYS  9
#define NUM_MODES 3

// ── Encoder 1 — Volume (constant across modes) ───────────────                                                                                                                                                                                                                                            
//   Moved to input-only pins 34/35 to free 18/19 for SPI CLK
#define ENC1_CLK  34
#define ENC1_DT   35
#define ENC1_SW   5                                                                                                                                                                                                 

// ── Encoder 2 — Mode-aware; SW click = mode change ───────────
#define ENC2_CLK  16
#define ENC2_DT   17
#define ENC2_SW   22   // was 2 (now TFT_DC)

// ── TFT (ST7789 240×240, defined here for reference) ─────────
//   Actual values passed via build_flags → TFT_eSPI
//   MOSI=23, SCLK=18, CS=15, DC=2, RST=4

// ── Debounce ──────────────────────────────────────────────────
#define DEBOUNCE_MS     30
#define ENC_DEBOUNCE_MS 50

// ── Display timing ────────────────────────────────────────────
#define LAST_ACTION_TTL_MS 2000   // how long "Last:" label stays visible