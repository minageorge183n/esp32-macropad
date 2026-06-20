#include "display.h"

TFT_eSPI tft = TFT_eSPI();

bool     screenDirty        = true;
char     lastActionLabel[32] = "";
uint32_t lastActionMs        = 0;
bool     lastActionVisible   = false;
uint8_t  lastActionKey       = 0xFF;

// ── Internal helpers ─────────────────────────────────────────

// Cell top-left pixel coords from key index (0-8)
static void cellOrigin(int idx, int& x, int& y) {
    int col = idx % 3;
    int row = idx / 3;
    x = col * (CELL_W + CELL_GAP);
    y = GRID_TOP + row * (CELL_H + CELL_GAP);
}

// ── Pixel icons ───────────────────────────────────────────────
// All drawn relative to (ox, oy) = top-left of a 14×14 bounding box
// centred horizontally inside the cell.

static void iconVolUp(int ox, int oy, uint16_t col) {
    // speaker body
    tft.fillRect(ox,     oy+3,  4, 6, col);
    tft.fillTriangle(ox+4, oy+3, ox+9, oy, ox+9, oy+12, col);
    // three arc dots (right side)
    tft.fillRect(ox+10, oy+1,  2, 2, col);
    tft.fillRect(ox+11, oy+5,  2, 2, col);
    tft.fillRect(ox+10, oy+9,  2, 2, col);
}

static void iconVolDown(int ox, int oy, uint16_t col) {
    tft.fillRect(ox,     oy+3,  4, 6, col);
    tft.fillTriangle(ox+4, oy+3, ox+9, oy, ox+9, oy+12, col);
    // one arc dot only
    tft.fillRect(ox+10, oy+5,  2, 2, col);
}

static void iconMute(int ox, int oy, uint16_t col) {
    tft.fillRect(ox,     oy+3,  4, 6, col);
    tft.fillTriangle(ox+4, oy+3, ox+9, oy, ox+9, oy+12, col);
    // X cross
    tft.drawLine(ox+10, oy+2,  ox+13, oy+5,  col);
    tft.drawLine(ox+13, oy+2,  ox+10, oy+5,  col);
    tft.drawLine(ox+10, oy+8,  ox+13, oy+11, col);
    tft.drawLine(ox+13, oy+8,  ox+10, oy+11, col);
}

static void iconPrev(int ox, int oy, uint16_t col) {
    // stop bar + left triangle
    tft.fillRect(ox, oy, 2, 12, col);
    tft.fillTriangle(ox+3, oy+6, ox+10, oy, ox+10, oy+12, col);
}

static void iconPlay(int ox, int oy, uint16_t col) {
    tft.fillTriangle(ox, oy, ox+12, oy+6, ox, oy+12, col);
}

static void iconNext(int ox, int oy, uint16_t col) {
    tft.fillTriangle(ox, oy, ox+8, oy+6, ox, oy+12, col);
    tft.fillRect(ox+9, oy, 2, 12, col);
}

static void iconCut(int ox, int oy, uint16_t col) {
    // scissors: two circles + crossing blades
    tft.drawCircle(ox+3,  oy+3,  3, col);
    tft.drawCircle(ox+3,  oy+10, 3, col);
    tft.drawLine(ox+5,  oy+5,  ox+13, oy+12, col);
    tft.drawLine(ox+5,  oy+9,  ox+13, oy+2,  col);
}

static void iconCopy(int ox, int oy, uint16_t col) {
    // back page
    tft.drawRoundRect(ox,   oy,   9, 11, 2, col);
    // front page (filled white gap then border)
    tft.fillRoundRect(ox+3, oy+3, 9, 11, 2, C_PANEL);
    tft.drawRoundRect(ox+3, oy+3, 9, 11, 2, col);
}

static void iconPaste(int ox, int oy, uint16_t col) {
    // clipboard body
    tft.drawRoundRect(ox+1, oy+2, 10, 12, 2, col);
    // clip tab
    tft.fillRoundRect(ox+4, oy,   5,  4,  1, col);
    // ruled lines
    tft.drawFastHLine(ox+3, oy+6,  7, col);
    tft.drawFastHLine(ox+3, oy+9,  7, col);
}

static void iconNone(int ox, int oy, uint16_t col) {
    // subtle dash
    tft.drawFastHLine(ox+4, oy+6, 6, col);
}

// Dispatch icon by ActionType + label
static void drawKeyIcon(const KeyAction& ka, int cx, int iy) {
    // cx = cell left edge, iy = icon top edge
    int ox = cx + (CELL_W - 14) / 2;   // centre the 14px icon horizontally
    uint16_t col = (ka.type == KEY_NONE) ? C_TEXT_TER : C_BLUE;

    if (ka.type == KEY_NONE) { iconNone(ox, iy, col); return; }

    // Match by label string (fast on small keymap)
    const char* L = ka.label;
    if      (strcmp(L, "Vol+")    == 0) { iconVolUp(ox,   iy, C_BLUE);  }
    else if (strcmp(L, "Vol-")    == 0) { iconVolDown(ox, iy, C_BLUE);  }
    else if (strcmp(L, "Mute")    == 0) { iconMute(ox,    iy, C_RED);   }
    else if (strcmp(L, "Prev")    == 0) { iconPrev(ox,    iy, C_GREEN); }
    else if (strcmp(L, "Play")    == 0) { iconPlay(ox,    iy, C_BLUE);  }
    else if (strcmp(L, "Next")    == 0) { iconNext(ox,    iy, C_GREEN); }
    else if (strcmp(L, "Cut")     == 0) { iconCut(ox,     iy, C_AMBER); }
    else if (strcmp(L, "Copy")    == 0) { iconCopy(ox,    iy, C_AMBER); }
    else if (strcmp(L, "Paste")   == 0) { iconPaste(ox,   iy, C_AMBER); }
    else if (strcmp(L, "Save")    == 0) {
        // floppy disk outline
        tft.drawRoundRect(ox, iy, 12, 13, 1, C_BLUE);
        tft.fillRect(ox+3, iy, 5, 4, C_BLUE);          // label slot
        tft.fillRect(ox+3, iy+8, 6, 5, C_PANEL_HI);   // data window
        tft.drawRect(ox+3, iy+8, 6, 5, C_BLUE);
    }
    else if (strcmp(L, "Undo")    == 0) {
        tft.drawArc(ox+6, iy+7, 6, 4, 120, 360, C_BLUE, C_PANEL);
        tft.fillTriangle(ox, iy+3, ox+5, iy, ox+5, iy+6, C_BLUE);
    }
    else if (strcmp(L, "Redo")    == 0) {
        tft.drawArc(ox+6, iy+7, 6, 4, 0, 240, C_BLUE, C_PANEL);
        tft.fillTriangle(ox+9, iy, ox+14, iy+3, ox+9, iy+6, C_BLUE);
    }
    else if (strcmp(L, "Format")  == 0 ||
             strcmp(L, "Comment") == 0 ||
             strcmp(L, "Find")    == 0 ||
             strcmp(L, "Terminal")== 0) {
        // generic code brackets <>
        tft.drawLine(ox+1, iy+4,  ox+4,  iy+7,  C_BLUE);
        tft.drawLine(ox+4, iy+7,  ox+1,  iy+10, C_BLUE);
        tft.drawLine(ox+10,iy+4,  ox+13, iy+7,  C_BLUE);
        tft.drawLine(ox+13,iy+7,  ox+10, iy+10, C_BLUE);
    }
    else if (strcmp(L, "TODO")    == 0 ||
             strcmp(L, "Debug")   == 0) {
        // terminal prompt >_
        tft.drawLine(ox+1, iy+4, ox+4, iy+7, C_GREEN);
        tft.drawLine(ox+4, iy+7, ox+1, iy+10, C_GREEN);
        tft.drawFastHLine(ox+6, iy+11, 6, C_GREEN);
    }
    else {
        iconNone(ox, iy, C_TEXT_TER);
    }
}

// ── Menu bar ──────────────────────────────────────────────────
static void drawMenuBar(uint8_t mode, bool bleConnected) {
    // Background panel
    tft.fillRoundRect(0, 0, SCREEN_W, MENUBAR_H, 10, C_PANEL);
    // Square off bottom corners
    tft.fillRect(0, 10, SCREEN_W, MENUBAR_H - 10, C_PANEL);

    // Traffic-light dots (left side)
    tft.fillCircle(14, MENUBAR_H / 2, 5, C_TL_RED);
    tft.fillCircle(27, MENUBAR_H / 2, 5, C_TL_AMBER);
    tft.fillCircle(40, MENUBAR_H / 2, 5, C_TL_GREEN);

    // Mode name, centred
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_TEXT, C_PANEL);
    tft.setTextSize(1);
    tft.drawString(enc2Modes[mode].label, SCREEN_W / 2, MENUBAR_H / 2 + 1);

    // BLE pill (right side)
    uint16_t bleDot = bleConnected ? C_GREEN : C_RED;
    tft.fillRoundRect(SCREEN_W - 36, 6, 32, 16, 8, C_PANEL_HI);
    tft.fillCircle(SCREEN_W - 29, MENUBAR_H / 2, 3, bleDot);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(bleConnected ? C_GREEN : C_RED, C_PANEL_HI);
    tft.drawString("BT", SCREEN_W - 24, MENUBAR_H / 2 + 1);
}

// ── 3×3 key grid ─────────────────────────────────────────────
static void drawGrid(uint8_t mode) {
    for (int idx = 0; idx < NUM_KEYS; idx++) {
        int cx, cy;
        cellOrigin(idx, cx, cy);

        const KeyAction& ka = keymap[mode][idx];
        bool empty    = (ka.type == KEY_NONE);
        bool isActive = (idx == lastActionKey && lastActionVisible);

        // Card background
        uint16_t bgCol = empty    ? C_BG :
                         isActive ? C_ACTIVE_BG : C_PANEL;
        uint16_t brCol = isActive ? C_BLUE : C_SEPARATOR;

        tft.fillRoundRect(cx, cy, CELL_W, CELL_H, CELL_R, bgCol);
        tft.drawRoundRect(cx, cy, CELL_W, CELL_H, CELL_R, brCol);

        if (empty) {
            // Subtle dash in centre
            iconNone(cx + (CELL_W - 14) / 2, cy + (CELL_H - 14) / 2, C_TEXT_TER);
            continue;
        }

        // Icon (upper half of cell, vertically centred in top 28px)
        int iconY = cy + 10;
        drawKeyIcon(ka, cx, iconY);

        // Label (lower portion)
        uint16_t textCol = isActive ? C_BLUE : C_TEXT;
        tft.setTextDatum(BC_DATUM);
        tft.setTextColor(textCol, bgCol);
        tft.setTextSize(1);
        tft.drawString(ka.label, cx + CELL_W / 2, cy + CELL_H - 6);
    }
}

// ── Encoder dock ──────────────────────────────────────────────
// Returns short label for an EncoderAction
static const char* encLabel(const EncoderAction& a) {
    if (a.type == ENC_NONE) return "-";
    if (a.type == ENC_MEDIA) {
        if (a.mediaKeycode == KEY_MEDIA_VOLUME_UP)      return "Vol+";
        if (a.mediaKeycode == KEY_MEDIA_VOLUME_DOWN)    return "Vol-";
        if (a.mediaKeycode == KEY_MEDIA_MUTE)           return "Mute";
        if (a.mediaKeycode == KEY_MEDIA_NEXT_TRACK)     return "Next";
        if (a.mediaKeycode == KEY_MEDIA_PREVIOUS_TRACK) return "Prev";
        if (a.mediaKeycode == KEY_MEDIA_PLAY_PAUSE)     return "Play";
        return "Med";
    }
    if (a.type == ENC_SINGLE) {
        if (a.keycode == KEY_TAB && a.modifiers[0] == 0)              return "Indent";
        if (a.keycode == KEY_TAB && a.modifiers[0] == KEY_LEFT_SHIFT) return "Undent";
        if (a.keycode == '/')                                          return "Cmnt";
        return "Key";
    }
    return "?";
}

static void drawDock(uint8_t mode) {
    // Pill background
    tft.fillRoundRect(0, DOCK_Y, SCREEN_W, DOCK_H, DOCK_R, C_PANEL);

    const EncoderMode& em = enc2Modes[mode];

    // Three equal sections: CW | CCW | Mode (each ~80px wide)
    // Divider positions
    const int divX[2] = { 80, 160 };
    for (int x : divX) {
        tft.drawFastVLine(x, DOCK_Y + 4, DOCK_H - 8, C_SEPARATOR);
    }

    int cy = DOCK_Y + DOCK_H / 2 + 1;

    // Section 0: CW
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(C_TEXT_SEC, C_PANEL);
    tft.setTextSize(1);
    tft.drawString("CW", 5, cy);
    tft.setTextColor(C_TEXT, C_PANEL);
    tft.drawString(encLabel(em.cw), 28, cy);

    // Section 1: CCW
    tft.setTextColor(C_TEXT_SEC, C_PANEL);
    tft.drawString("CC", divX[0] + 5, cy);
    tft.setTextColor(C_TEXT, C_PANEL);
    tft.drawString(encLabel(em.ccw), divX[0] + 28, cy);

    // Section 2: Mode badge + "Mode"
    tft.setTextColor(C_TEXT_SEC, C_PANEL);
    tft.drawString("SW", divX[1] + 5, cy);
    tft.setTextColor(C_BLUE, C_PANEL);
    tft.drawString("Mode", divX[1] + 28, cy);

    // Mode index badge — pill in the far right corner
    char badge[4];
    snprintf(badge, sizeof(badge), "M%d", mode);
    tft.fillRoundRect(SCREEN_W - 22, DOCK_Y + 4, 20, DOCK_H - 8, 5, C_PANEL_HI);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_TEXT, C_PANEL_HI);
    tft.drawString(badge, SCREEN_W - 12, cy);
}

// ── Toast pill ────────────────────────────────────────────────
static void drawToast() {
    // Always erase the toast zone first
    tft.fillRect(0, TOAST_Y, SCREEN_W, TOAST_H + 2, C_BG);
    if (!lastActionVisible) return;

    // Centre the pill
    const int pillW = 164;
    const int pillX = (SCREEN_W - pillW) / 2;

    tft.fillRoundRect(pillX, TOAST_Y, pillW, TOAST_H, TOAST_R, C_PANEL);
    tft.drawRoundRect(pillX, TOAST_Y, pillW, TOAST_H, TOAST_R, C_SEPARATOR);

    // Blue dot
    tft.fillCircle(pillX + 10, TOAST_Y + TOAST_H / 2, 3, C_FLASH);

    // Label
    char buf[36];
    snprintf(buf, sizeof(buf), "%.30s", lastActionLabel);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(C_TEXT, C_PANEL);
    tft.setTextSize(1);
    tft.drawString(buf, pillX + 18, TOAST_Y + TOAST_H / 2 + 1);
}

// ── Public API ────────────────────────────────────────────────
void displayInit() {
    tft.init();
    tft.setRotation(2);
    tft.fillScreen(C_BG);
    tft.setTextDatum(MC_DATUM);
}

void displayDraw(uint8_t mode, bool bleConnected) {
    tft.startWrite();
    tft.fillScreen(C_BG);
    drawMenuBar(mode, bleConnected);
    drawGrid(mode);
    drawDock(mode);
    drawToast();
    tft.endWrite();
    screenDirty = false;
}

void displaySetLastAction(const char* label, uint8_t keyIdx) {
    strncpy(lastActionLabel, label, sizeof(lastActionLabel) - 1);
    lastActionLabel[sizeof(lastActionLabel) - 1] = '\0';
    lastActionMs      = millis();
    lastActionVisible = true;
    lastActionKey     = keyIdx;
    screenDirty       = true;
}

void displayTick(uint8_t mode, bool bleConnected) {
    if (lastActionVisible && (millis() - lastActionMs) >= LAST_ACTION_TTL_MS) {
        lastActionVisible = false;
        lastActionKey     = 0xFF;

        // Redraw only the affected areas (no full repaint)
        tft.startWrite();

        // Re-draw whichever key lost its active highlight
        if (lastActionKey < NUM_KEYS) {
            int cx, cy;
            cellOrigin(lastActionKey, cx, cy);
            const KeyAction& ka = keymap[mode][lastActionKey];
            bool empty = (ka.type == KEY_NONE);
            tft.fillRoundRect(cx, cy, CELL_W, CELL_H, CELL_R, empty ? C_BG : C_PANEL);
            tft.drawRoundRect(cx, cy, CELL_W, CELL_H, CELL_R, C_SEPARATOR);
            if (!empty) {
                drawKeyIcon(ka, cx, cy + 10);
                tft.setTextDatum(BC_DATUM);
                tft.setTextColor(C_TEXT, C_PANEL);
                tft.setTextSize(1);
                tft.drawString(ka.label, cx + CELL_W / 2, cy + CELL_H - 6);
            }
        }

        drawToast();   // erase the pill
        tft.endWrite();
    }
}
