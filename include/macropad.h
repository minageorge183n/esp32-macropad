#pragma once

// ── Direct key pins (one GPIO per switch) ─────────────────────
//  Removed 34 and 35 (input-only, no pull-up support)
const int KEY_PINS[9] = {13, 12, 14, 27, 26, 25, 33, 32, 15};
//                                                         ^^^ was 35

#define NUM_KEYS  9
#define MODE_PIN  4       // was 34 — must be a pull-up capable pin
//               ^

#define NUM_MODES 3

// ── Debounce ──────────────────────────────────────────────────
#define DEBOUNCE_MS 50