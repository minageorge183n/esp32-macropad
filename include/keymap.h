#pragma once
#include <BleKeyboard.h> 
#include "macropad.h"
#include "keys.h"


// ── Mapping helpers ───────────────────────────────────────────
#define SINGLE(label, mod, key) \
  { KEY_SINGLE, label, {mod, 0, 0}, key, nullptr, nullptr }

#define MEDIA(label, key) \
  { KEY_MEDIA,  label, {0, 0, 0},   0,   key,     nullptr }

#define STRING(label, str) \
  { KEY_STRING, label, {0, 0, 0},   0,   nullptr, str     }

#define NONE \
  { KEY_NONE,   "-",   {0, 0, 0},   0,   nullptr, nullptr }

// ── Key layout reference ──────────────────────────────────────
// [ 0 ][ 1 ][ 2 ]
// [ 3 ][ 4 ][ 5 ]
// [ 6 ][ 7 ][ 8 ]

const KeyAction keymap[NUM_MODES][NUM_KEYS] = {

  // ── MODE 0 : Media ───────────────────────────────────────
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

  // ── MODE 1 : Dev Tools ───────────────────────────────────
  {
    SINGLE("Save",     KEY_LEFT_GUI, 's'),
    SINGLE("Undo",     KEY_LEFT_GUI, 'z'),
    SINGLE("Redo",     KEY_LEFT_GUI, 'y'),

    SINGLE("Comment",  KEY_LEFT_GUI, '/'),
    SINGLE("Terminal", KEY_LEFT_GUI, '`'),
    SINGLE("Find",     KEY_LEFT_GUI, 'f'),

    STRING("TODO",   "// TODO: "),
    STRING("Debug",  "console.log()"),
    STRING("Email",  "you@email.com"),
  },
  // ── MODE 2 : Custom ──────────────────────────────────────
  {
    NONE, NONE, NONE,
    NONE, NONE, NONE,
    NONE, NONE, NONE,
  },
};

