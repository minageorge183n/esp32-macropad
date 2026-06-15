#include "display.h"

TFT_eSPI tft = TFT_eSPI();

bool     screenDirty       = true;
char     lastActionLabel[32] = "";
uint32_t lastActionMs       = 0;
bool     lastActionVisible  = false;

// ─────────────────────────────────────────────────────────────
void displayInit() {
  tft.init();
  tft.setRotation(2);          // portrait, connector at bottom
  tft.fillScreen(C_BG);
  tft.setTextDatum(MC_DATUM);  // middle-centre default
}

// ── Top bar: mode name + BLE indicator ───────────────────────
static void drawTopBar(uint8_t mode, bool bleConnected) {
  tft.fillRect(0, 0, SCREEN_W, TOP_BAR_H, C_ACCENT);
  tft.setTextColor(C_BG, C_ACCENT);
  tft.setTextSize(1);
  tft.setFreeFont(1);    // built-in GLCD

  // Mode index badge
  char badge[8];
  snprintf(badge, sizeof(badge), "M%d", mode);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(badge, 4, TOP_BAR_H / 2);

  // Mode label, centred
  tft.setTextDatum(MC_DATUM);
  tft.drawString(enc2Modes[mode].label, SCREEN_W / 2, TOP_BAR_H / 2);

  // BLE dot
  uint16_t dotColor = bleConnected ? C_BLE_ON : C_BLE_OFF;
  tft.fillCircle(SCREEN_W - 10, TOP_BAR_H / 2, 6, dotColor);
}

// ── 3×3 key grid ─────────────────────────────────────────────
static void drawGrid(uint8_t mode) {
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      int idx = row * 3 + col;
      int x   = col * CELL_W;
      int y   = GRID_Y + row * CELL_H;

      const KeyAction& ka = keymap[mode][idx];
      bool empty = (ka.type == KEY_NONE);

      // Cell background
      tft.fillRect(x + 1, y + 1, CELL_W - 2, CELL_H - 2,
                   empty ? C_BG : C_PANEL);

      // Cell border
      tft.drawRect(x, y, CELL_W, CELL_H, C_BORDER);

      // Key number (small, top-left of cell)
      tft.setTextColor(C_DIM, empty ? C_BG : C_PANEL);
      tft.setTextDatum(TL_DATUM);
      tft.setTextSize(1);
      char num[4];
      snprintf(num, sizeof(num), "%d", idx);
      tft.drawString(num, x + 3, y + 3);

      // Label, centred
      tft.setTextColor(empty ? C_DIM : C_TEXT, empty ? C_BG : C_PANEL);
      tft.setTextDatum(MC_DATUM);
      tft.drawString(ka.label, x + CELL_W / 2, y + CELL_H / 2);
    }
  }
}

// ── Encoder 2 row ─────────────────────────────────────────────
static void drawEncRow(uint8_t mode) {
  tft.fillRect(0, ENC_ROW_Y, SCREEN_W, ENC_ROW_H, C_PANEL);
  tft.drawRect(0, ENC_ROW_Y, SCREEN_W, ENC_ROW_H, C_BORDER);

  const EncoderMode& em = enc2Modes[mode];

  // Helper to get a short label from an EncoderAction
  // We re-use the enc2Modes label strings for CW/CCW since EncoderAction
  // has no label field — derive from keycode for common cases.
  auto encLabel = [](const EncoderAction& a) -> const char* {
    if (a.type == ENC_NONE)  return "-";
    if (a.type == ENC_MEDIA) {
      if (a.mediaKeycode == KEY_MEDIA_VOLUME_UP)       return "Vol+";
      if (a.mediaKeycode == KEY_MEDIA_VOLUME_DOWN)     return "Vol-";
      if (a.mediaKeycode == KEY_MEDIA_MUTE)            return "Mute";
      if (a.mediaKeycode == KEY_MEDIA_NEXT_TRACK)      return "Next";
      if (a.mediaKeycode == KEY_MEDIA_PREVIOUS_TRACK)  return "Prev";
      if (a.mediaKeycode == KEY_MEDIA_PLAY_PAUSE)      return "Play";
      return "Med";
    }
    if (a.type == ENC_SINGLE) {
      if (a.keycode == KEY_TAB && a.modifiers[0] == 0)             return "Indent";
      if (a.keycode == KEY_TAB && a.modifiers[0] == KEY_LEFT_SHIFT) return "Undent";
      if (a.keycode == '/')                                         return "Cmnt";
      return "Key";
    }
    return "?";
  };

  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextSize(1);

  // Three equal sections: CW | CCW | SW
  const char* labels[3] = { encLabel(em.cw), encLabel(em.ccw), "Mode↑" };
  const char* icons[3]  = { "\x18", "\x19", "\x0F" }; // up/down/bullet in GLCD

  for (int i = 0; i < 3; i++) {
    int cx = (SCREEN_W / 6) + i * (SCREEN_W / 3);
    int cy = ENC_ROW_Y + ENC_ROW_H / 2;
    if (i < 2) {
      // Draw a tiny arrow indicator
      tft.setTextColor(C_ACCENT, C_PANEL);
      tft.setTextDatum(MR_DATUM);
      tft.drawString(i == 0 ? "CW" : "CC", cx - 2, cy);
    }
    tft.setTextColor(C_TEXT, C_PANEL);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(labels[i], cx + 2, cy);
  }
}

// ── Last action bar ───────────────────────────────────────────
static void drawLastAction() {
  tft.fillRect(0, LAST_Y, SCREEN_W, LAST_H, C_BG);
  if (!lastActionVisible) return;

  tft.setTextColor(C_FLASH, C_BG);
  tft.setTextDatum(ML_DATUM);
  tft.setTextSize(1);
  char buf[48];
  snprintf(buf, sizeof(buf), "\x10 %s", lastActionLabel); // ► symbol
  tft.drawString(buf, 4, LAST_Y + LAST_H / 2);
}

// ── Full redraw ───────────────────────────────────────────────
void displayDraw(uint8_t mode, bool bleConnected) {
  tft.startWrite();    // hold SPI CS low for the whole frame
  drawTopBar(mode, bleConnected);
  drawGrid(mode);
  drawEncRow(mode);
  drawLastAction();
  tft.endWrite();
  screenDirty = false;
}

// ── Set last action + trigger redraw ─────────────────────────
void displaySetLastAction(const char* label) {
  strncpy(lastActionLabel, label, sizeof(lastActionLabel) - 1);
  lastActionLabel[sizeof(lastActionLabel) - 1] = '\0';
  lastActionMs      = millis();
  lastActionVisible = true;
  screenDirty       = true;
}

// ── TTL ticker — call every loop ─────────────────────────────
void displayTick(uint8_t mode, bool bleConnected) {
  if (lastActionVisible && (millis() - lastActionMs) >= LAST_ACTION_TTL_MS) {
    lastActionVisible = false;
    // Only redraw the last-action bar, not the whole screen
    tft.startWrite();
    drawLastAction();
    tft.endWrite();
  }
}