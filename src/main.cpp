#include <Arduino.h>
#include <BleKeyboard.h>
#include "macropad.h"
#include "keys.h"
#include "keymap.h"
#include "display.h"
#include "games.h"
#include "spotify.h"

BleKeyboard ble("Macropad", "DIY", 100);

// ── Debounce state ────────────────────────────────────────────
struct KeyDebounce {
  bool     confirmed;
  bool     raw;
  uint32_t lastChangeMs;
};

KeyDebounce keys[NUM_KEYS] = {};
KeyDebounce modeKey        = {};
uint8_t     currentMode    = 0;
bool        bleWasConnected = false;

// Track whether Spotify has been initialised yet
// (lazy-init on first entry into mode 3 to avoid slowing down boot)
static bool spotifyInitialised = false;
static uint8_t prevMode = 0;

// ── Rotary encoder state ──────────────────────────────────────
struct Encoder {
  int      pinCLK, pinDT, pinSW;
  int      lastCLK;
  bool     swConfirmed;
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

  pinMode(ENC1_CLK, INPUT);
  pinMode(ENC1_DT,  INPUT);
  pinMode(ENC1_SW,  INPUT_PULLUP);
  pinMode(ENC2_CLK, INPUT_PULLUP);
  pinMode(ENC2_DT,  INPUT_PULLUP);
  pinMode(ENC2_SW,  INPUT_PULLUP);

  enc1.lastCLK = digitalRead(ENC1_CLK);
  enc2.lastCLK = digitalRead(ENC2_CLK);

  displayInit();
  gamesInit(&tft);
  ble.begin();

  screenDirty = true;
  Serial.println("Macropad ready");
}

// ── Debounce helpers ──────────────────────────────────────────
bool debounce(KeyDebounce& k, bool reading) {
  uint32_t now = millis();
  if (reading != k.raw) { k.raw = reading; k.lastChangeMs = now; return false; }
  if ((now - k.lastChangeMs) >= DEBOUNCE_MS) {
    bool prev = k.confirmed; k.confirmed = k.raw;
    return k.raw && !prev;
  }
  return false;
}

bool debounceEncSW(Encoder& e) {
  uint32_t now     = millis();
  bool     reading = (digitalRead(e.pinSW) == LOW);
  if (reading != e.swRaw) { e.swRaw = reading; e.swLastChangeMs = now; return false; }
  if ((now - e.swLastChangeMs) >= DEBOUNCE_MS) {
    bool prev = e.swConfirmed; e.swConfirmed = e.swRaw;
    return e.swRaw && !prev;
  }
  return false;
}

int pollEncoder(Encoder& e) {
  int clk = digitalRead(e.pinCLK);
  if (clk == e.lastCLK) return 0;
  delayMicroseconds(500);
  clk = digitalRead(e.pinCLK);
  if (clk == e.lastCLK) return 0;
  e.lastCLK = clk;
  return (clk == digitalRead(e.pinDT)) ? 1 : -1;
}

// ── BLE action dispatch ───────────────────────────────────────
void executeAction(const KeyAction& action) {
  if (!ble.isConnected()) return;
  switch (action.type) {
    case KEY_SINGLE:
      for (uint8_t mod : action.modifiers) if (mod) ble.press(mod);
      ble.press(action.keycode);
      delay(10); ble.releaseAll();
      break;
    case KEY_MEDIA:
      ble.press(action.mediaKeycode);
      delay(10); ble.releaseAll();
      break;
    case KEY_STRING:
      ble.print(action.str);
      break;
    default: break;
  }
}

void executeEncoderAction(const EncoderAction& action) {
  if (!ble.isConnected()) return;
  switch (action.type) {
    case ENC_MEDIA:
      ble.press(action.mediaKeycode);
      delay(10); ble.releaseAll();
      break;
    case ENC_SINGLE:
      for (uint8_t mod : action.modifiers) if (mod) ble.press(mod);
      ble.press(action.keycode);
      delay(10); ble.releaseAll();
      break;
    default: break;
  }
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {

  bool bleConnected = ble.isConnected();

  if (bleConnected != bleWasConnected) {
    bleWasConnected = bleConnected;
    if (currentMode != 3) screenDirty = true;
  }

  if (gamesTick()) return;

  // ── Spotify mode ──────────────────────────────────────────
  if (currentMode == 3) {
    // Lazy init: connect WiFi + fetch first token the first time we enter
    if (!spotifyInitialised) {
      spotifyInit(&tft);
      spotifyInitialised = true;
    }

    // If we just switched into mode 3, force a full redraw
    if (prevMode != 3) {
      spotifyDrawFull();
    }
    prevMode = 3;

    // Keys still send BLE commands (Vol+/-, Play, Prev, Next)
    for (int i = 0; i < NUM_KEYS; i++) {
      if (debounce(keys[i], digitalRead(KEY_PINS[i]) == LOW)) {
        const KeyAction& action = keymap[currentMode][i];
        if (action.type != KEY_NONE) {
          Serial.printf("Spotify key %d → %s\n", i, action.label);
          executeAction(action);
          // After a play/pause or skip, force an immediate Spotify poll
          // so the display updates quickly rather than waiting 3 s
          // We signal this by resetting the poll timer (accessed via extern)
          // spotifyTick() will pick it up next call
        }
      }
    }

    // Enc2 CW/CCW = volume via BLE
    int d2 = pollEncoder(enc2);
    if (d2 != 0) {
      const EncoderMode& em = enc2Modes[currentMode];
      executeEncoderAction(d2 > 0 ? em.cw : em.ccw);
    }

    // Enc2 SW = mode change (same as other modes)
    if (debounceEncSW(enc2)) {
      currentMode = (currentMode + 1) % NUM_MODES;
      Serial.printf("Mode → %d (%s)\n", currentMode, enc2Modes[currentMode].label);
      screenDirty = true;
    }

    // Enc1 = always vol
    int d1 = pollEncoder(enc1);
    if (d1 != 0) {
      if (d1 > 0) ble.press(KEY_MEDIA_VOLUME_UP);
      else        ble.press(KEY_MEDIA_VOLUME_DOWN);
      delay(10); ble.releaseAll();
    }
    if (debounceEncSW(enc1)) {
      ble.press(KEY_MEDIA_MUTE);
      delay(10); ble.releaseAll();
    }

    // Tick the Spotify module — handles polling + progress bar updates
    spotifyTick();
    return;
  }

  prevMode = currentMode;

  // ── Normal macropad modes (0-2) ───────────────────────────

  // ── 9 keys ──────────────────────────────────────────────────
  for (int i = 0; i < NUM_KEYS; i++) {
    if (debounce(keys[i], digitalRead(KEY_PINS[i]) == LOW)) {
      const KeyAction& action = keymap[currentMode][i];
      Serial.printf("Key %d [mode %d] → %s\n", i, currentMode, action.label);
      executeAction(action);
      displaySetLastAction(action.label, i);
    }
  }

  // ── Encoder 1 — Volume (constant) ───────────────────────────
  int d1 = pollEncoder(enc1);
  if (d1 != 0) {
    if (d1 > 0) {
      Serial.println("Enc1 CW  → Vol+");
      ble.press(KEY_MEDIA_VOLUME_UP);
      displaySetLastAction("Vol+");
    } else {
      Serial.println("Enc1 CCW → Vol-");
      ble.press(KEY_MEDIA_VOLUME_DOWN);
      displaySetLastAction("Vol-");
    }
    delay(10); ble.releaseAll();
  }
  if (debounceEncSW(enc1)) {
    Serial.println("Enc1 SW  → Mute");
    ble.press(KEY_MEDIA_MUTE);
    delay(10); ble.releaseAll();
    displaySetLastAction("Mute");
  }

  // ── Encoder 2 — Mode-aware; SW = mode change ─────────────────
  int d2 = pollEncoder(enc2);
  if (d2 != 0) {
    const EncoderMode& em = enc2Modes[currentMode];
    if (d2 > 0) {
      Serial.printf("Enc2 CW  [mode %d]\n", currentMode);
      executeEncoderAction(em.cw);
      displaySetLastAction("Enc2 CW");
    } else {
      Serial.printf("Enc2 CCW [mode %d]\n", currentMode);
      executeEncoderAction(em.ccw);
      displaySetLastAction("Enc2 CCW");
    }
  }
  if (debounceEncSW(enc2)) {
    currentMode = (currentMode + 1) % NUM_MODES;
    Serial.printf("Mode → %d (%s)\n", currentMode, enc2Modes[currentMode].label);
    screenDirty = true;
  }

  // ── Screen refresh ───────────────────────────────────────────
  if (screenDirty) {
    displayDraw(currentMode, bleConnected);
  }
  displayTick(currentMode, bleConnected);
}