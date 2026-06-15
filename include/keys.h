#pragma once
#include <BleKeyboard.h>

enum ActionType {
  KEY_NONE,
  KEY_SINGLE,
  KEY_MEDIA,
  KEY_STRING,
};

struct KeyAction {
  ActionType     type;
  const char*    label;
  uint8_t        modifiers[3];
  uint8_t        keycode;
  const uint8_t* mediaKeycode;
  const char*    str;
};

enum EncoderActionType {
  ENC_NONE,
  ENC_MEDIA,
  ENC_SINGLE,
};

struct EncoderAction {
  EncoderActionType type;
  uint8_t           modifiers[2];
  uint8_t           keycode;
  const uint8_t*    mediaKeycode;
};

struct EncoderMode {
  const char*   label;
  EncoderAction cw;
  EncoderAction ccw;
  EncoderAction press;   // Enc2 SW = mode change, but press action still fires first
};