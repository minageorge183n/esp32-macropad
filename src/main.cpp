#include <Arduino.h>
#include <BleKeyboard.h>
#include "macropad.h"
#include "keys.h"
#include "keymap.h"

BleKeyboard ble("Macropad", "DIY", 100);

// ── Debounce state ────────────────────────────────────────────
struct KeyDebounce {
  bool     confirmed;      // last stable (debounced) state
  bool     raw;            // current raw reading
  uint32_t lastChangeMs;   // when raw last changed
};

KeyDebounce keys[NUM_KEYS] = {};
KeyDebounce modeKey        = {};
uint8_t     currentMode    = 0;

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_KEYS; i++)
    pinMode(KEY_PINS[i], INPUT_PULLUP);
  pinMode(MODE_PIN, INPUT_PULLUP);

  ble.begin();
  Serial.println("Macropad ready");
}

// ── Debounce helper ───────────────────────────────────────────
// Returns true on the moment a key becomes stable-pressed (leading edge only)
 // ── Debounce helper ───────────────────────────────────────────
// Returns true on the moment a key becomes stable-pressed (leading edge only)
bool debounce(KeyDebounce& k, bool reading) {
  uint32_t now = millis();

  if (reading != k.raw) {
    k.raw          = reading;
    k.lastChangeMs = now;
    return false;                   // state just changed — wait for it to settle
  }

  if ((now - k.lastChangeMs) >= DEBOUNCE_MS) {
    bool wasConfirmed = k.confirmed;
    k.confirmed       = k.raw;
    return k.raw && !wasConfirmed;  // true only on press leading edge
  }

  return false;
}

// ── Execute action ────────────────────────────────────────────
void executeAction(const KeyAction& action) {
  if (!ble.isConnected()) return;

  switch (action.type) {
    case KEY_SINGLE:
      for (uint8_t mod : action.modifiers)
        if (mod) ble.press(mod);
      ble.press(action.keycode);
      delay(10);
      ble.releaseAll();
      break;

    case KEY_MEDIA:
      ble.press(action.mediaKeycode);
      delay(10);
      ble.releaseAll();
      break;

    case KEY_STRING:
      ble.print(action.str);
      break;

    default: break;
  }
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {
  // Mode button
  if (debounce(modeKey, digitalRead(MODE_PIN) == LOW)) {
    currentMode = (currentMode + 1) % NUM_MODES;
    Serial.printf("Mode → %d\n", currentMode);
  }

  // 9 keys
  for (int i = 0; i < NUM_KEYS; i++) {
    if (debounce(keys[i], digitalRead(KEY_PINS[i]) == LOW)) {
      const KeyAction& action = keymap[currentMode][i];
      Serial.printf("Key %d [mode %d] → %s\n", i, currentMode, action.label);
      executeAction(action);
    }
  }
}
