#pragma once
#include <BleKeyboard.h>

enum ActionType {
  KEY_NONE,
  KEY_SINGLE,   // normal keystroke, with optional modifiers
  KEY_MEDIA,    // media key (uint8_t* pointer type in BLE lib)
  KEY_STRING,   // types a string
};

struct KeyAction {
  ActionType     type;
  const char*    label;
  uint8_t        modifiers[3];    // up to 3 modifier keys
  uint8_t        keycode;         // normal keycode
  const uint8_t* mediaKeycode;    // media keycode (pointer)
  const char*    str;             // string to type
};