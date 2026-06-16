#pragma once
#include <TFT_eSPI.h>

// ── Board config ─────────────────────────────────────────────
// Play field: 10 cols × 20 rows, each cell 10px → 100×200 px
// Centred horizontally with side panel for score
#define TET_COLS       10
#define TET_ROWS       20
#define TET_CELL       11          // px (1 px gap included)
#define TET_ORIGIN_X   10          // left edge of board
#define TET_ORIGIN_Y   20          // top edge
#define TET_PANEL_X    (TET_ORIGIN_X + TET_COLS * TET_CELL + 6)   // ~126
#define TET_PANEL_W    (240 - TET_PANEL_X - 2)

// Colours (RGB565)
#define TET_C_BG       0x1082
#define TET_C_BORDER   0x4208
#define TET_C_EMPTY    0x2104
#define TET_C_TEXT     0xFFFF
#define TET_C_GHOST    0x4208

// Piece colours by type index (I O T S Z J L)
static const uint16_t TET_COLOURS[7] = {
    0x07FF,  // I  cyan
    0xFFE0,  // O  yellow
    0xC81F,  // T  purple
    0x07E0,  // S  green
    0xF800,  // Z  red
    0x001F,  // J  blue
    0xFD20,  // L  orange
};

struct TetPiece {
    int8_t x, y;
    uint8_t type;
    uint8_t rot;
};

class TetrisGame {
public:
    void init(TFT_eSPI* tft);
    bool update();         // returns false when user exits
    void draw();

    bool isOver()      const { return _gameOver; }
    bool wantsExit()   const { return _wantsExit; }

private:
    TFT_eSPI* _tft;
    uint8_t   _board[TET_ROWS][TET_COLS];  // colour index (0 = empty)
    TetPiece  _cur, _ghost;
    uint32_t  _lastDrop;
    uint32_t  _dropMs;
    uint32_t  _lastInput;
    int       _score, _level, _lines;
    bool      _gameOver, _wantsExit;
    bool      _boardDirty;  // full board repaint needed

    // Input debounce timestamps
    uint32_t  _btnLast[9];

    void _spawnPiece();
    bool _valid(int8_t x, int8_t y, uint8_t type, uint8_t rot) const;
    void _place();
    int  _clearLines();
    void _calcGhost();

    void _drawBoard();
    void _drawPiece(const TetPiece& p, bool erase = false, bool ghost = false);
    void _drawCell(int x, int y, uint16_t colour);
    void _drawPanel();
    void _drawGameOver();
    bool _btnEdge(int idx, uint32_t repeatMs = 0);
};
