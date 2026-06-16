#include "games.h"
#include "macropad.h"
#include "snake_game.h"
#include "tetris_game.h"

// ── Forward declarations ──────────────────────────────────────
static void _launchSplash(const char* name, uint16_t colour);

// ── Module state ─────────────────────────────────────────────
static TFT_eSPI* _tft = nullptr;

static SnakeGame  _snake;
static TetrisGame _tetris;

enum class ActiveGame { NONE, SNAKE, TETRIS };
static ActiveGame _active = ActiveGame::NONE;

// Combo tracking
struct ComboTracker {
    uint8_t  keyA, keyB;   // KEY_PINS indices
    uint32_t holdStart;
    bool     fired;
};
static ComboTracker _combos[2] = {
    { 0, 2, 0, false },   // SNAKE  : top-left + top-right
    { 6, 8, 0, false },   // TETRIS : bot-left + bot-right
};

// ─────────────────────────────────────────────────────────────
void gamesInit(TFT_eSPI* tft) {
    _tft = tft;
}

// ─────────────────────────────────────────────────────────────
bool gamesTick() {
    // ── If a game is running, hand control to it ──────────────
    if (_active == ActiveGame::SNAKE) {
        bool running = _snake.update();
        if (!running || _snake.wantsExit()) {
            _active = ActiveGame::NONE;
            // Signal main.cpp to do a full redraw of the macropad UI
            extern bool screenDirty;
            screenDirty = true;
            // Reset combo so it doesn't re-fire on key release
            _combos[0].holdStart = 0;
            _combos[0].fired = true;
        }
        return true;
    }
    if (_active == ActiveGame::TETRIS) {
        bool running = _tetris.update();
        if (!running || _tetris.wantsExit()) {
            _active = ActiveGame::NONE;
            extern bool screenDirty;
            screenDirty = true;
            _combos[1].holdStart = 0;
            _combos[1].fired = true;
        }
        return true;
    }

    // ── No game running — check combos ────────────────────────
    uint32_t now = millis();

    // Combo 0 → Snake
    {
        auto& c = _combos[0];
        bool held = (digitalRead(KEY_PINS[c.keyA]) == LOW &&
                     digitalRead(KEY_PINS[c.keyB]) == LOW);
        if (!held) {
            c.holdStart = 0;
            c.fired = false;
        } else if (!c.fired) {
            if (c.holdStart == 0) c.holdStart = now;
            if ((now - c.holdStart) >= COMBO_HOLD_MS) {
                c.fired = true;
                _launchSplash("SNAKE", 0x07E0);
                _snake.init(_tft);
                _active = ActiveGame::SNAKE;
                return true;
            }
        }
    }

    // Combo 1 → Tetris
    {
        auto& c = _combos[1];
        bool held = (digitalRead(KEY_PINS[c.keyA]) == LOW &&
                     digitalRead(KEY_PINS[c.keyB]) == LOW);
        if (!held) {
            c.holdStart = 0;
            c.fired = false;
        } else if (!c.fired) {
            if (c.holdStart == 0) c.holdStart = now;
            if ((now - c.holdStart) >= COMBO_HOLD_MS) {
                c.fired = true;
                _launchSplash("TETRIS", 0x07FF);
                _tetris.init(_tft);
                _active = ActiveGame::TETRIS;
                return true;
            }
        }
    }

    return false;  // no game running
}

// ── 300 ms splash screen when launching ──────────────────────
static void _launchSplash(const char* name, uint16_t colour) {
    _tft->fillScreen(0x1082);
    _tft->setTextColor(colour, 0x1082);
    _tft->setTextDatum(MC_DATUM);
    _tft->setTextSize(3);
    _tft->drawString(name, 120, 100);
    _tft->setTextSize(1);
    _tft->setTextColor(0xFFFF, 0x1082);
    _tft->drawString("Loading...", 120, 135);
    delay(300);
}
