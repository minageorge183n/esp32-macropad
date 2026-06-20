#pragma once
#include <BleKeyboard.h>
#include "macropad.h"
#include "keys.h"

#define SINGLE(label, mod, key) \
  { KEY_SINGLE, label, {mod, 0, 0}, key, nullptr, nullptr }

#define MEDIA(label, key) \
  { KEY_MEDIA,  label, {0, 0, 0},   0,   key,     nullptr }

#define STRING(label, str) \
  { KEY_STRING, label, {0, 0, 0},   0,   nullptr, str     }

#define NONE \
  { KEY_NONE,   "-",   {0, 0, 0},   0,   nullptr, nullptr }

#define EMEDIA(key)          { ENC_MEDIA,  {0, 0},   0,   key     }
#define ESINGLE(mod, key)    { ENC_SINGLE, {mod, 0}, key, nullptr }
#define ESINGLE2(m1,m2,k)   { ENC_SINGLE, {m1, m2}, k,   nullptr }
#define ENONE                { ENC_NONE,   {0, 0},   0,   nullptr }

// Key layout:
// [ 0 ][ 1 ][ 2 ]
// [ 3 ][ 4 ][ 5 ]
// [ 6 ][ 7 ][ 8 ]

const KeyAction keymap[NUM_MODES][NUM_KEYS] = {

  // MODE 0 : Media
  {
    MEDIA("Vol+",  KEY_MEDIA_VOLUME_UP),
    MEDIA("Vol-",  KEY_MEDIA_VOLUME_DOWN),
    MEDIA("Mute",  KEY_MEDIA_MUTE),

    MEDIA("Prev",  KEY_MEDIA_PREVIOUS_TRACK),
    MEDIA("Play",  KEY_MEDIA_PLAY_PAUSE),
    MEDIA("Next",  KEY_MEDIA_NEXT_TRACK),

    SINGLE("Cut",   KEY_LEFT_GUI, 'x'),
    SINGLE("Copy",  KEY_LEFT_GUI, 'c'),
    SINGLE("Paste", KEY_LEFT_GUI, 'v'),
  },

  // MODE 1 : Dev Tools
  {
    SINGLE("Save",     KEY_LEFT_GUI, 's'),
    SINGLE("Undo",     KEY_LEFT_GUI, 'z'),
    SINGLE("Redo",     KEY_LEFT_GUI, 'y'),

    SINGLE("Comment",  KEY_LEFT_GUI, '/'),
    SINGLE("Terminal", KEY_LEFT_GUI, '`'),
    SINGLE("Find",     KEY_LEFT_GUI, 'f'),

    STRING("TODO",   "// TODO: "),
    STRING("Debug",  "console.log()"),
    SINGLE("Format", KEY_LEFT_ALT,   'f'),
  },

  // MODE 2 : Custom
  {
    NONE, NONE, NONE,
    NONE, NONE, NONE,
    NONE, NONE, NONE,
  },

  // MODE 3 : Spotify
  // Keys mirror Mode 0 media row — BLE controls the host device,
  // WiFi + Spotify API just drives the display.
  {
    MEDIA("Vol+",  KEY_MEDIA_VOLUME_UP),
    MEDIA("Vol-",  KEY_MEDIA_VOLUME_DOWN),
    MEDIA("Mute",  KEY_MEDIA_MUTE),

    MEDIA("Prev",  KEY_MEDIA_PREVIOUS_TRACK),
    MEDIA("Play",  KEY_MEDIA_PLAY_PAUSE),
    MEDIA("Next",  KEY_MEDIA_NEXT_TRACK),

    NONE, NONE, NONE,
  },
};

const EncoderMode enc2Modes[NUM_MODES] = {

  // MODE 0 : track scrubbing
  {
    "Media Scrub",
    EMEDIA(KEY_MEDIA_NEXT_TRACK),
    EMEDIA(KEY_MEDIA_PREVIOUS_TRACK),
    EMEDIA(KEY_MEDIA_PLAY_PAUSE),
  },

  // MODE 1 : indent / unindent
  {
    "Indent",
    ESINGLE(0, KEY_TAB),
    ESINGLE2(KEY_LEFT_SHIFT, 0, KEY_TAB),
    ESINGLE(KEY_LEFT_GUI, '/'),
  },

  // MODE 2 : custom
  {
    "Custom",
    ENONE,
    ENONE,
    ENONE,
  },

  // MODE 3 : Spotify display (encoder = vol, SW = play/pause)
  {
    "Spotify",
    EMEDIA(KEY_MEDIA_VOLUME_UP),
    EMEDIA(KEY_MEDIA_VOLUME_DOWN),
    EMEDIA(KEY_MEDIA_PLAY_PAUSE),
  },
};