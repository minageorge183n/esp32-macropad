#pragma once
#include <TFT_eSPI.h>

// ── Grid config for 240x240 TFT ──────────────────────────────
// Play area: 200x200 px, centred, leaving room for score bar
#define SNG_CELL      10          // px per cell
#define SNG_COLS      20          // 200 / 10
#define SNG_ROWS      20          // 200 / 10
#define SNG_ORIGIN_X  20          // left margin
#define SNG_ORIGIN_Y  30          // below score bar
#define SNG_MAX_LEN   (SNG_COLS * SNG_ROWS)

// Colours
#define SNG_C_BG      0x1082
#define SNG_C_BORDER  0x4208
#define SNG_C_HEAD    0x07FF      // cyan
#define SNG_C_BODY    0x07E0      // green
#define SNG_C_FOOD    0xF800      // red
#define SNG_C_TEXT    0xFFFF

// Speed
#define SNG_START_MS  250         // ms per step
#define SNG_MIN_MS    80

struct SnakePoint { int8_t x, y; };

class SnakeGame {
public:
    void init(TFT_eSPI* tft);
    // Returns true while game is still running, false when player exits/dies
    bool update();   // call every loop; handles timing internally
    void draw();

    bool isOver() const { return _gameOver; }
    bool wantsExit() const { return _wantsExit; }

private:
    TFT_eSPI* _tft;
    SnakePoint _body[SNG_MAX_LEN];
    int        _len;
    int8_t     _dx, _dy;         // current direction
    int8_t     _ndx, _ndy;       // queued direction
    SnakePoint _food;
    int        _score;
    uint32_t   _stepMs;
    uint32_t   _lastStep;
    bool       _gameOver;
    bool       _wantsExit;
    bool       _needsRedraw;

    // Previous state for dirty-rect updates
    SnakePoint _prevTail;
    bool       _atePrevStep;

    void       _placeFood();
    void       _drawCell(int8_t x, int8_t y, uint16_t colour);
    void       _drawScoreBar();
    void       _drawGameOver();
    void       _fullRedraw();
};
