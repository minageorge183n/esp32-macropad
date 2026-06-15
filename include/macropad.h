#pragma once

// ── Direct key pins (8 switches) ─────────────────────────────
const int KEY_PINS[9] = {13, 12, 14, 27, 26, 25, 33, 32,15};

#define NUM_KEYS  9
#define NUM_MODES 3

// ── Mode button ───────────────────────────────────────────────
#define MODE_PIN  4

// ── Encoder 1 — Volume (constant across modes) ───────────────
#define ENC1_CLK  18
#define ENC1_DT   19
#define ENC1_SW   5

// ── Encoder 2 — Mode-aware ────────────────────────────────────
#define ENC2_CLK  16
#define ENC2_DT   17
#define ENC2_SW   2

// ── Debounce ──────────────────────────────────────────────────
#define DEBOUNCE_MS     50
#define ENC_DEBOUNCE_MS 30    // encoders need a tighter window
