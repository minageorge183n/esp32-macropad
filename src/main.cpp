#include <Arduino.h>
#include <BleKeyboard.h>
#include "macropad.h"
#include "keys.h"
#include "keymap.h"

BleKeyboard ble("Macropad", "DIY", 100);

// ── Debounce state (keys + mode button) ──────────────────────
struct KeyDebounce {
  bool     confirmed;
  bool     raw;
  uint32_t lastChangeMs;
};

KeyDebounce keys[NUM_KEYS] = {};
KeyDebounce modeKey        = {};
uint8_t     currentMode    = 0;

// ── Rotary encoder state ──────────────────────────────────────
struct Encoder {
  int      pinCLK, pinDT, pinSW;
  int      lastCLK;            // last stable CLK level
  bool     swConfirmed;        // debounced SW state
  bool     swRaw;
  uint32_t swLastChangeMs;
};

Encoder enc1 = { ENC1_CLK, ENC1_DT, ENC1_SW, HIGH, false, false, 0 };
Encoder enc2 = { ENC2_CLK, ENC2_DT, ENC2_SW, HIGH, false, false, 0 };

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_KEYS; i++)
    pinMode(KEY_PINS[i], INPUT_PULLUP);
  pinMode(MODE_PIN, INPUT_PULLUP);

  // Encoders
  pinMode(ENC1_CLK, INPUT_PULLUP);
  pinMode(ENC1_DT,  INPUT_PULLUP);
  pinMode(ENC1_SW,  INPUT_PULLUP);
  pinMode(ENC2_CLK, INPUT_PULLUP);
  pinMode(ENC2_DT,  INPUT_PULLUP);
  pinMode(ENC2_SW,  INPUT_PULLUP);

  enc1.lastCLK = digitalRead(ENC1_CLK);
  enc2.lastCLK = digitalRead(ENC2_CLK);

  ble.begin();
  Serial.println("Macropad ready");
}

// ── Key debounce ──────────────────────────────────────────────
// Returns true on the stable leading edge (press moment only).
bool debounce(KeyDebounce& k, bool reading) {
  uint32_t now = millis();
  if (reading != k.raw) {
    k.raw          = reading;
    k.lastChangeMs = now;
    return false;
  }
  if ((now - k.lastChangeMs) >= DEBOUNCE_MS) {
    bool prev   = k.confirmed;
    k.confirmed = k.raw;
    return k.raw && !prev;
  }
  return false;
}

// ── Encoder SW debounce ───────────────────────────────────────
bool debounceEncSW(Encoder& e) {
  uint32_t now     = millis();
  bool     reading = (digitalRead(e.pinSW) == LOW);
  if (reading != e.swRaw) {
    e.swRaw          = reading;
    e.swLastChangeMs = now;
    return false;
  }
  if ((now - e.swLastChangeMs) >= DEBOUNCE_MS) {
    bool prev      = e.swConfirmed;
    e.swConfirmed  = e.swRaw;
    return e.swRaw && !prev;
  }
  return false;
}

// ── Encoder rotation poll ─────────────────────────────────────
// Returns +1 (CW), -1 (CCW), or 0 (no change).
int pollEncoder(Encoder& e) {
  int clk = digitalRead(e.pinCLK);
  if (clk == e.lastCLK) return 0;      // no edge

  delayMicroseconds(500);              // brief settle
  clk = digitalRead(e.pinCLK);        // re-read after settle
  if (clk == e.lastCLK) return 0;

  e.lastCLK   = clk;
  int dt      = digitalRead(e.pinDT);
  return (clk != dt) ? 1 : -1;        // CW vs CCW
}

// ── Execute a KeyAction ───────────────────────────────────────
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

// ── Execute an EncoderAction ──────────────────────────────────
void executeEncoderAction(const EncoderAction& action) {
  if (!ble.isConnected()) return;

  switch (action.type) {
    case ENC_MEDIA:
      ble.press(action.mediaKeycode);
      delay(10);
      ble.releaseAll();
      break;

    case ENC_SINGLE:
      for (uint8_t mod : action.modifiers)
        if (mod) ble.press(mod);
      ble.press(action.keycode);
      delay(10);
      ble.releaseAll();
      break;

    default: break;
  }
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {

  // ── Mode button ─────────────────────────────────────────────
  if (debounce(modeKey, digitalRead(MODE_PIN) == LOW)) {
    currentMode = (currentMode + 1) % NUM_MODES;
    Serial.printf("Mode → %d (%s)\n", currentMode, enc2Modes[currentMode].label);
  }

  // ── 8 keys ──────────────────────────────────────────────────
  for (int i = 0; i < NUM_KEYS; i++) {
    if (debounce(keys[i], digitalRead(KEY_PINS[i]) == LOW)) {
      const KeyAction& action = keymap[currentMode][i];
      Serial.printf("Key %d [mode %d] → %s\n", i, currentMode, action.label);
      executeAction(action);
    }
  }

  // ── Encoder 1 — Volume (constant) ───────────────────────────
  int d1 = pollEncoder(enc1);
  if (d1 != 0) {
    if (d1 > 0) {
      Serial.println("Enc1 CW  → Vol+");
      ble.press(KEY_MEDIA_VOLUME_UP);
    } else {
      Serial.println("Enc1 CCW → Vol-");
      ble.press(KEY_MEDIA_VOLUME_DOWN);
    }
    delay(10);
    ble.releaseAll();
  }
  if (debounceEncSW(enc1)) {
    Serial.println("Enc1 SW  → Mute");
    ble.press(KEY_MEDIA_MUTE);
    delay(10);
    ble.releaseAll();
  }

  // ── Encoder 2 — Mode-aware ───────────────────────────────────
  int d2 = pollEncoder(enc2);
  if (d2 != 0) {
    const EncoderMode& em = enc2Modes[currentMode];
    if (d2 > 0) {
      Serial.printf("Enc2 CW  [mode %d]\n", currentMode);
      executeEncoderAction(em.cw);
    } else {
      Serial.printf("Enc2 CCW [mode %d]\n", currentMode);
      executeEncoderAction(em.ccw);
    }
  }
  if (debounceEncSW(enc2)) {
    Serial.printf("Enc2 SW  [mode %d]\n", currentMode);
    executeEncoderAction(enc2Modes[currentMode].press);
  }
}
